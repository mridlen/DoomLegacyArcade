# Cabinet Link: networked cabinets (Phases 1 and 3 built; 2 and 4 still a plan)

*Part of the DoomLegacy arcade cabinet build. Read before touching `d_link.c`/`d_link.h`,
`tools/linktest.sh`, the `HAVE_LINK` build option, or `i_tcp.c` socket code. Written 2026-09-13 as a
plan before any code; each phase's section is replaced with the record of what was built, what broke
and how it was verified as it lands. **Phase 1 (identity, pairing, presence) and Phase 3 (invites
and linked games) are built** — see "Phase 1 — what was built" and "Phase 3 — what was built".
Shared scores (Phase 2) are still plan.*

See `CLAUDE.md` for the build, headless verification and the cross-cutting rules index.

---

## What it does, in plain terms

Two or more cabinets on the same home network join into a group.

- **They share one set of high scores.** A record set on the Pi shows up on the laptop's attract
  screen and intermission, with its record demo, and the other way round. A cabinet that was
  switched off catches up the next time it is on.
- **A multiplayer game on one cabinet invites the others.** When somebody starts Deathmatch (or a
  Campaign) on the laptop, a cabinet sitting on its attract screen shows **PRESS FIRE TO JOIN**
  along with the countdown the laptop's join screen is running. Whoever presses in on the Pi plays
  in the same game, on the Pi's own screen and controls. **The invite interrupts the menus** — if
  someone on the Pi is halfway through picking a game, the invite replaces the menu, because the
  likeliest reason they are there is that they are trying to set up the same game. A cabinet that is
  in the middle of a game, or has someone signing the high score board, is not interrupted.
- **Nothing gets in without the passcode.** Cabinets talk over an encrypted connection, prove to
  each other that they were set up with the same passcode, and remember each other's identity so a
  different machine cannot pretend to be one of them later.
- **One cabinet is the master**; the others connect to it. If the master is off, every cabinet
  still plays normally on its own and syncs when the master comes back.

It is built for any number of cabinets up to the engine's 32 player limit, tested with two. Nearly
all of it costs the same for two as for thirty-two; the places where it does not are listed under
[Scaling past two](#scaling-past-two-cabinets).

**Out of scope for the first version:** play over the internet, joining a game already in
progress, and sharing the operator's audit counters (each cabinet's money and play counts stay its
own).

---

## What the engine already gives us

Found by reading the code on 2026-09-13; every claim below has a file to check.

- **32 players, 4 per machine.** `MAXPLAYERS 32` (`doomdef.h`), `MAXSPLITSCREENPLAYERS 4`,
  `MAXNETNODES 32` (`d_net.h`). So the limit is **32 players in total with at most 4 per cabinet** —
  32x1, 16x2 and 8x4 all fit, and so does any mix (4+4+2+1...). It is not a fixed panels-per-cabinet
  shape.
- **Every game is already a network game.** Solo and local multiplayer run through the client/server
  code in `d_clisrv.c`; local splitscreen sets `netgame`. A remote cabinet is "another node" to code
  that already handles nodes — the client join packet carries `num_node_players`
  (`clientconfig_pak_t`), and the arcade's four-panel work generalised the per-node player mask.
  This is the single biggest reason the plan is feasible.
- **The transport is plain UDP, unauthenticated.** `SOCK_Send`/`SOCK_Get` (`i_tcp.c`), port 5029,
  `MAXPACKETLENGTH 1450`. Any machine on the network can send packets that the netcode parses. The
  parsers were written for friendly LAN parties in the 2000s and were never hardened. **This, not
  the score sync, is the main security exposure**, and the plan closes it (Phase 3).
- **The netcode already checks wads by MD5** (`d_netfil.c`) and **downloads missing ones by
  default** (`cv_download_files` and `cv_download_savegame`, both default `1`). A file written to
  disk because a network peer said so has no place on a cabinet; linked games must force both off.
- **A desync detector exists**: `Consistency()` (`d_clisrv.c`) sums player positions and the random
  index every tic, and `SV_consistency_fault` repairs or drops the node.
- **Scoring is single player only.** `HS_Scored_Game` (`hs_stuff.c`) refuses anything with `netgame`
  or a second human, so **networked games are never scored** and never produce a record demo. Score
  sharing and networked play are therefore independent features that happen to share a connection.
- **The score files are plain text with append-only fields**, written atomically:
  - `highscores.dat` — `game map skill tics category startmap`, the best per board key.
  - `runs.dat` — `game startmap endmap skill category tics initials`, the ranked run board.
  - `demos/<game>_<map>_sk<N>_<cat>.lmp` (and `<game>_ep<N>_...` for Survival) — one record demo per
    key. The whole directory is about **1.1MB for 102 demos** on the laptop, the largest 73KB, so
    transferring all of it is trivial even over the Pi 3's Wi-Fi.
  - Neither file records **when** or **on which cabinet** a record was set. Sync needs both
    (see Phase 2).
- **The join screen** (`m_menu.c`, `M_Join_Open` / `M_Join_Start`, ~3437) already has the shape the
  invite needs: a countdown (`cv_jointime`), per-panel press-in and setup, and a start callback. The
  Deathmatch and Campaign routes go through `M_Arcade_MP_Go`, which sets `cv_wait_players` to the
  joined count — a remote cabinet's players just add to that count.
- **Threads**: `r_threads.c` uses `SDL_CreateThread`/`SDL_mutex`, so a background network thread
  has a portable precedent (Linux, Pi, Windows).
- **No TLS library is linked today.** Nothing in the `Makefile` or `tools/build.sh` mentions one.

---

## Architecture

Two separate channels, because they have opposite needs.

```
                 ┌──────────────── Link channel ────────────────┐
                 │  TCP + TLS 1.3, port 5030, always on          │
   Pi (member) ──┤  pairing / auth, presence, invites,           ├── Laptop (master)
                 │  score + demo sync                            │
                 └───────────────────────────────────────────────┘
                 ┌──────────────── Game channel ────────────────┐
                 │  existing Legacy UDP netcode, port 5029,      │
   Pi (client) ──┤  only during a linked game, every packet      ├── cabinet that started
                 │  encrypted + authenticated with a per-game key│   the game (host)
                 └───────────────────────────────────────────────┘
```

### The link channel (new)

- **One long-lived TLS connection per member, to the master.** Star topology: the master relays
  presence and invites between members. With two cabinets it is simply a direct connection. With
  many, it is still N-1 connections, not N².
- **Runs on its own thread.** A TLS handshake, a DNS lookup or a slow Wi-Fi write must never stall a
  frame (`no-added-input-latency` applies to the attract screen too — a hitch there is a hitch on a
  demo). The link thread owns the sockets and OpenSSL; the main thread talks to it through two
  mutex-protected message queues, polled **once per tic** from `D_DoomLoop`.
  - **The main thread owns all game state.** The link thread never touches `hs_table`, `hs_runs`,
    menus or cvars. It hands the main thread "here is the peer's manifest" or "a demo finished
    downloading to `demos/x.lmp.tmp`", and the main thread merges, renames and saves. This is the
    `R_TLS` lesson from the render threads applied up front: decide which thread owns what before
    writing the first line, not after the first corruption.
- **Messages are length-framed binary**: 4-byte length, 1-byte type, little-endian fields, a
  protocol version in `HELLO`, and a hard maximum size per type (a demo chunk is at most 16KB; a
  manifest at most a few hundred KB). Anything oversized or unknown closes the connection. No text
  parser, no JSON.
- **Liveness**: a `PING` every 5 seconds; 15 seconds of silence drops the peer and marks it offline.
  Reconnection backs off 1s, 2s, 4s ... capped at 60s, so a master that is switched off costs the
  members nothing noticeable.

### Roles

- **Master** listens on 5030. **Member** connects to the master's address from its config.
- **Game host is not the master.** The cabinet whose player started the game hosts it (its engine
  runs `SV_SpawnServer` as it does today); the others connect to it directly over UDP. The master
  only brokers the invite and hands out addresses. So the Pi can start a Deathmatch that the laptop
  joins without the game being routed through anything.
- **Addresses** are hostnames or IPs in the config. A hostname such as `laptop.local` survives the
  Pi getting a new DHCP address (mDNS via avahi/nss-mdns on both Linux machines). Automatic
  discovery is deliberately left out of the first version: it is exactly the kind of unauthenticated
  broadcast this plan is removing.

### Cabinet state, as the link sees it

Each cabinet publishes one of:

| state | meaning | receives invites? |
| --- | --- | --- |
| `IDLE` | attract cycle (pages or demos), no menu open | **yes** |
| `MENU` | someone is in the menus over attract | **yes — the menu is closed for the invite** |
| `JOINING` | its own join screen is up, nobody remote in it yet | **only for the same game** (see below) |
| `HOSTING` | its own join screen is up and a remote cabinet has joined it | no |
| `PLAYING` | in a level, intermission or finale | no |
| `SIGNING` | initials entry | no — never interrupt someone signing the board |
| `DEVMODE` | an operator session | no — an operator mid-change is not a player |

Menus are interrupted on purpose (decided 2026-09-13): someone at the Pi's menus while the laptop
opens a Deathmatch is most likely trying to start that same game, and a Pi that sits silently in its
menus while the laptop counts down is the confusing outcome.

Presence also carries the cabinet's name, its panel count (`cv_localplayers`), its build
(`DLA_VERSION`) and the list of games it can run with their wad fingerprints — so an invite for TNT
is only shown on cabinets that have `TNT.WAD`, and never becomes a "wad mismatch" error on screen.

---

## Security

The goal is that a device on the same network that does not know the passcode cannot: read or
change scores, push demos, see or answer invites, or send a single packet the game will parse.

### Identity

- On first enabling the link, each cabinet generates a key pair (ECDSA P-256) and a self-signed
  certificate, stored in `legacyhome/link/` with mode 0600. The **cabinet ID is the SHA-256 of the
  public key**; the operator page shows a short form of it (e.g. `7F3A-91C2`).
- Keys, pins and the passcode are written with `M_Atomic_Write_Open`/`Close` — the power-cut rule.
- **The passcode does not go in `config.cfg`.** That file has a tracked copy at
  `cabinet/legacyhome/config.cfg`, and a passcode must never reach git. It lives in
  `legacyhome/link/link.cfg` (0600), which is gitignored along with the rest of `link/`.

### The handshake, every connection

1. **TLS 1.3 only**, both sides present their certificate (mutual TLS). OpenSSL's normal chain
   verification is replaced by a callback that records the peer's fingerprint.
2. **The member checks the master's fingerprint against its pin** before sending anything else. A
   mismatch drops the connection and puts "MASTER IDENTITY CHANGED" on the operator page — it does
   not fall back to asking again.
3. **The member proves it knows the passcode, bound to this exact TLS session.** Both sides take a
   TLS exporter value (`SSL_export_keying_material`, label `"dla-link-auth"`), and the member sends
   `HMAC-SHA256( key = PBKDF2(passcode, salt = both cabinet IDs), msg = exporter || "member" )`.
   Because the exporter is unique to the session, a machine sitting in the middle of two connections
   cannot relay the proof from one to the other.
4. **Only then does the master prove it back** (the same with `"master"`). The order matters: a
   stranger who connects to the master and guesses wrong learns nothing it could take away and crack
   later — each wrong guess costs it a live attempt.
5. **Rate limiting**: three failed proofs from one address locks it out for 60 seconds, logged to the
   console and counted.
6. On success the master pins the member's fingerprint too. **Changing the passcode on the master
   revokes every member at once**, since their next proof fails; "Forget paired cabinets" on the
   operator page clears the pins.

### Address allow list

**Each cabinet only talks to the addresses it is told to** (decided 2026-09-13). The master has a
list of member addresses (`link_allow` in `link.cfg`); a member only ever connects to its configured
master and accepts nothing inbound on 5030. A connection from anywhere else is closed **before the
TLS handshake starts**, so a stranger on the network never reaches OpenSSL, let alone the passcode
check. The game channel already drops any address outside the session.

This pairs with the setup Mark intends for an untrusted Wi-Fi network: give the cabinets fixed
addresses on **wired Ethernet** and list only those. The allow list is a filter, not a replacement
for the passcode — an address can be borrowed by another device on the same network, the passcode
cannot.

### The first connection, and the honest limit of a passcode

On the very first connection the member has no pin for the master yet, so it trusts the first
master it reaches (the same model as SSH's "trust this host?"). The operator page on both cabinets
shows the two short cabinet IDs; comparing them once confirms nobody is in the middle, and from then
on the pin makes that permanent.

A passcode on its own has one theoretical gap: someone *already* intercepting traffic at the exact
moment of the very first pairing could collect one proof and try passcodes offline. **Decided
2026-09-13: a passcode is sufficient** — together with the allow list, wired Ethernet and a
passcode the operator page asks to be at least 10 characters, the attacker has to be inside the
wired network during a one-off setup. The heavier fix for that gap (a password-authenticated key
exchange such as SPAKE2) is not planned.

### The game channel

- When a linked game starts, the host generates a random 256-bit **session key** and sends it only
  to the cabinets that joined, over their authenticated TLS link.
- A shim at the bottom of `SOCK_Send`/`SOCK_Get` (`i_tcp.c`) seals every UDP packet with
  **ChaCha20-Poly1305** (OpenSSL EVP): an 8-byte counter as the nonce, a 16-byte tag. On receive the
  tag is checked **before the netcode sees a byte**; a failed tag, a replayed counter (64-packet
  sliding window per sender) or an unknown address is dropped silently. The stock parsers are then
  only ever fed packets from a cabinet that proved the passcode.
- **Cost**: roughly 40 bytes and a few microseconds per packet on the Pi 3. That is measured, not
  assumed, before Phase 3 lands (see "input latency" below). Local-only games do not use the socket
  and are untouched.
- In a linked session the engine also **forces off** `cv_download_files` and
  `cv_download_savegame`, refuses stock `connect` / `askinfo` broadcasts from anything outside the
  session, and **only opens the UDP port for the duration of a linked game**.

### What the link does *not* protect

A cabinet that knows the passcode is trusted. If one cabinet's scores are corrupted — the Pi's SD
card has already shown it does not survive power loss (`pi-intermittent-crash-open`) — sync would
spread the damage. So Phase 2 adds sanity rules: **a record without its demo is not accepted, the
demo's header must parse, and tics must be positive and plausible**. Replaying every received demo
headlessly to confirm its time is possible later with `-timedemo`/`-synclog`, but not in v1.

### Library

**OpenSSL 3**, optional at build time as `HAVE_LINK=1`:

- Fedora (`openssl-devel`), Raspberry Pi OS (`libssl-dev`), MSYS2 (`mingw-w64-ucrt-x86_64-openssl`)
  and the GitHub runners all ship it.
- `tools/build.sh` / `tools/build.ps1` probe it by test-compiling, the way they probe SDL2_mixer.
  Without it the link code compiles out and the operator page hides — so a build machine without
  OpenSSL still builds the game, which is the rule those scripts exist to keep.
- `build.ps1` already derives the Windows DLLs from import tables, so `libssl-3-x64.dll` and
  `libcrypto-3-x64.dll` are picked up with no hardcoded list.
- mbedTLS was the alternative (small, easy to vendor); it loses on having to be vendored and kept
  patched, when every target already has a maintained OpenSSL.

---

## Shared high scores and demos

### Sync is a merge of state, not a replay of events

Each side sends a **manifest** of what it holds; each side computes the merged result with the
**same pure function** and fetches whatever it is missing. There is no queue of "records to send",
so nothing is lost to a power cut, nothing is applied twice, and a cabinet that was off for a month
catches up exactly like one that was off for a minute.

For that to converge, the merge must be:

- **commutative** — merge(A, B) = merge(B, A), so both cabinets reach the same board;
- **idempotent** — merging the same manifest again changes nothing;
- **associative** — so three cabinets syncing through a master in any order agree.

That holds only if **every comparison is a total order**. Two records with equal tics from two
cabinets must not be "equal" or each cabinet keeps its own and they flip back and forth for ever.
Ties break by `(tics, set_time, cabinet_id, initials)`: the record set first wins, and the cabinet
ID settles the rest.

### File format changes (append-only, as before)

- `highscores.dat` gains `set_time cabinet_id demo_sha256`. `set_time` is Unix seconds from the
  cabinet that set it. Old lines read back with `set_time 0` (oldest) and the local cabinet's ID.
- `runs.dat` gains `set_time cabinet_id`. The run board is a union of both cabinets' entries,
  deduplicated on the whole tuple, sorted by the existing `HS_Board_Sort` rules, trimmed to the
  board's size.
- A **board epoch** in a new header line. `clearhighscores` bumps it. **Without it a cleared board
  comes straight back from the other cabinet on the next sync**; with it, entries from an older epoch
  are discarded and the clear spreads instead. Clearing is allowed on the master only, so two
  cabinets cannot clear at once and race. **Clearing on the master clears every cabinet** (decided
  2026-09-13) — which is also why a stranger on the network must not be able to send one.
  - Nobody needs to try this by hand on the real cabinets: it is exactly the case the two-instance
    headless test in Phase 2 covers, against scratch copies of `legacyhome`, and that test is where
    it gets proven. A member that was switched off during the clear is part of that test too — it
    must lose its old records when it reconnects, not resurrect them.
- Clock sanity: a Pi without a real-time clock boots in 1970 until NTP answers. A `set_time` before
  2026-01-01 is sent as unknown (`0`) rather than as a real time, so it can never beat a genuine one
  on a tie.

### What is compared, and what is refused

A record is only comparable with the same **game content and rules**, and today's key
(`doom2`, `doom2+packname`) does not prove that — two cabinets can hold different versions of a pack
with the same file name.

- The manifest carries, per game ID, a **fingerprint of the loaded wads** (the MD5s the netcode
  already computes, cached by size and mtime so the Pi does not rehash `DOOM2.WAD` on every boot).
  Game IDs whose fingerprints differ are skipped and listed on the operator page.
- The **build must match**: same `DLA_VERSION` for sync and for games. Two builds from different
  commits can simulate differently without any version number changing, which desyncs a netgame and
  makes a shared demo play out wrong. The operator page says which cabinet needs updating.
- The **ranked ruleset must match**: `HS_Apply_Ranked_Ruleset` pins most gameplay settings, and the
  handshake hashes the ones it does not (the same list `tools/cfgaudit.py` reports). A mismatch is a
  warning on the operator page and pauses score sync, because a record under different rules is a
  different board.

### Demo transfer

- Demos travel with their `highscores.dat` entry: when the merge picks a remote record, the local
  cabinet requests that demo by SHA-256.
- Written to `demos/<name>.lmp.tmp`, hash checked, header checked, then **renamed over** the old file
  on the main thread. Leftover `.tmp` files are deleted at startup.
- **On Windows a rename over an open file fails**, and the attract cycle may be playing that very
  demo. The main thread retries the rename after the demo ends rather than failing the sync.
- The attract cycle needs no change: it already builds its demo list from the score table.

### When sync runs

On connect, then whenever a board changes (`HS_Save` / `HS_Runs_Save` nudge the link thread), and
every 10 minutes as a backstop. **Never during a scored run**: the main thread holds received merges
until the cabinet is back to `IDLE`, so a record appearing mid-run cannot change the target a player
is chasing or race the local `HS_LevelExit` write.

---

## Invites and networked games

### Which games invite

- **Deathmatch** — always.
- **Campaign** — yes (decided 2026-09-13), because the join screen is what decides between a solo
  run and coop today; a remote cabinet's player pressing in is the same as a second local panel
  pressing in: the game becomes coop and, as now, unranked. If nobody remote presses in, the solo
  run is scored exactly as it is now — sending an invite must not make a run unranked.
- **Single Level** — never. It is scored single player and has no join screen.
- **Multiplayer → Start Game** (the operator's hand-tuned page) — not in v1.

### The flow

1. **Laptop:** someone picks Deathmatch. The join screen opens as now and the link sends `INVITE`
   (game ID, category, episode/map, skill, host cabinet name, *seconds remaining*) to every cabinet
   in `IDLE` or `MENU` that can run the game. Time is sent as a duration, never a clock time — the
   two machines' clocks do not agree.
2. **Pi:** a banner: `DEATHMATCH ON LAPTOP — PRESS FIRE TO JOIN — 17`.
   - From the attract screen, the attract demo keeps playing underneath.
   - **From the menus, the menu is closed first** (`M_Clear_Menus`) and the banner shown over the
     attract screen. The banner cannot simply sit on top of an open menu: panel buttons are
     translated into menu movement (`M_Cabinet_Menu_Key`), so fire would select a menu row instead of
     joining. Whatever the person was choosing is abandoned the way Escape abandons it — in
     particular a half-finished guided control setup must be left exactly as a backed-out one is,
     and that is checked, not assumed.
   - A press on panel N opens that panel's own join cell with the same colour / crosshair / controls
     setup as the local join screen, which becomes a full join page on the Pi once anyone presses.
     The Pi's state goes to `JOINING`.
   - If nobody presses before the countdown ends, the banner goes and the Pi is on its attract
     screen. The menu is not reopened: that person saw the invite and let it go.
3. **Both:** each cabinet sends `JOIN_STATUS` as its panels press in and lock. The laptop's join
   screen shows `PI: 2 IN` beside its own cells; the Pi's shows `LAPTOP: 1 IN`.
   **View cells stay per cabinet** — a remote player never takes a quarter of the laptop's screen.
4. **Start**, when the countdown ends or every pressed-in panel on *every* cabinet has locked:
   the laptop sends `START` with its UDP address, port and the session key to each cabinet that has
   players in, then calls `M_Arcade_MP_Go` with `cv_wait_players` set to the **total** across
   cabinets. Each joined cabinet runs `D_Set_Panel`/`D_Set_View_Cell`/`D_Set_Join_Count` for its own
   panels and connects as a client with `num_node_players` = its own count.
5. **If a cabinet does not arrive** within 10 seconds, the host starts with whoever is present
   (`cv_wait_timeout` set for linked games — the arcade already learned that a wait with no timeout
   hangs the cabinet), and the late cabinet shows `COULD NOT JOIN` and returns to attract.
6. **A cabinet nobody pressed on** just drops the banner when the countdown ends.

### During and after

- Each cabinet's idle timeout and arcade-death rules apply **to its own players**. When all of a
  cabinet's players have left, that cabinet disconnects and returns to attract; the game carries on
  for everyone else. This has to be checked against `G_Idle_Timeout_Check` and
  `G_Arcade_Death_Check`, which were written for one machine.
- When the host's game ends (time limit, everyone gone), clients receive the server shutdown and go
  back to attract through `Command_ExitGame_f`, the one funnel that resets leftover state.
- If the host loses power mid-game, clients time out (`server_timeout_handler`) and do the same.
- **Two cabinets opening the same game at nearly the same time** is the case interrupting menus is
  for, so it must end in one game, not two. The master orders invites as it receives them. A cabinet
  on its own join screen (`JOINING`, nobody remote in it yet) that receives an invite for the **same
  game ID and category** turns into a join of the earlier game: its panels that already pressed in
  stay in, with their colour / crosshair / controls choices, and its own countdown is dropped for
  the host's. The master breaks an exact tie by cabinet ID, so both cabinets agree who hosts.
  - An invite for a *different* game (Deathmatch against a Campaign, or another wad) does not touch a
    cabinet already on its join screen — those people have chosen, and both games run separately.
  - Once a remote cabinet has joined a join screen (`HOSTING`), that cabinet can no longer be
    converted, so two cabinets can never each end up waiting on the other.

### Input latency

The rule is that a fix must never tax every keypress. Two separate things to measure:

- **The encryption shim** must add nothing measurable to a tic. Compare `-timedemo` style timing and
  a per-packet microsecond counter on the Pi 3, shim on and off.
- **The network itself** adds the round trip for players on the client cabinets — that is what
  networked play is, not a regression. But **players on the host must feel exactly what a local
  multiplayer game feels like today**, and that is measured by comparing when a ticcmd built on the
  host is applied, local game against linked game.
- The Pi 3 has 2.4GHz Wi-Fi only. The docs will recommend Ethernet for play; sync works fine on
  Wi-Fi.

---

## Operator page

**Built (2026-09-13)** — *Arcade Options → Cabinet Link*, `-devmode` only like the rest of Arcade
Options. Mark: "we need a better way of managing the cabinet link other than the console". Three
pages, all in the `CABINET LINK` block of `m_menu.c`:

- **`CabinetLinkDef`** — Role (fire, or left/right: off → master → member), Name, Passcode (shown as
  "SET, n CHARACTERS", never the text), Master (a member) or Allowed (a master), Port, Forget paired
  cabinets (fire twice). Below the rows, `LK_Drawer( y, y_end )`: running or why not, this cabinet's
  ID, and every peer with its status and reason.
- **`LinkTextDef`** — an on-screen keyboard: three rows of 13 keys and an action row (character set,
  SPACE, DELETE, CANCEL, DONE). Stick moves, fire types, *use* deletes. Sets are capitals, lower case,
  symbols; a field gets only the sets and characters it can hold (a port: digits). **The menu font has
  no lowercase and its "white" and grey are indistinguishable on screen**, so typed lowercase letters
  are drawn **red**, the set key says UPPER/LOWER/SYMBOL (not abc/ABC, which would draw the same), and
  "LOWER CASE LETTERS" heads the grid in that set — all found by looking at an OpenGL capture, where
  the first version showed `LINK1cd` as seven identical capitals. A passcode is typed afresh.
- **`LinkAllowDef`** — a master's allow list, then **ADD AN ADDRESS**, then every cabinet recently
  refused for not being on the list (the link already keeps the last eight refusals, with address and
  reason): **ALLOW 192.168.1.68**, one press. That is the intended way to set a master up — nobody has
  to know a member's address. Fire twice on an allowed address removes it. A member backs off up to 60 s
  between attempts, which the page says.

Every change goes through **`LK_Setting_Set` / `LK_Allow_Add` / `LK_Allow_Remove` / `LK_Forget_Pins`**
(`d_link.h`): validate, save `link.cfg`, restart the link. `link_set` and `link_forget` were rewritten
on top of them, so the page and the console cannot drift apart.

**Keys are taken raw, before `M_Cabinet_Menu_Key`** (`M_Link_Page_Key`, hooked in `M_Responder`
right after the join screen's hook, for the same reason). The laptop's panels are keyboard keys — `a`,
`e`, `h`, `n`, `o`, `t` and more are buttons — so a typed letter and a panel button can be the same
key. A key bound to a control is always the control; anything else a keyboard sends is typed.

`LK_Ticker` used to return at once for a cabinet with the link off, which also skipped `-linkstatus`;
it now prints status in every role, and runs the test key script before the role check (a cabinet
with the link off is exactly the one this page switches on).

**Verified**:
- `tools/linktest.sh menusetup`: two cabinets with **no `link.cfg`**, set up entirely by button presses
  through the input queue (`-linktest -linkkeys "<tokens>"`: `open`, `u d l r f b` for panel 1's
  stick, fire and use, `esc enter bs`, `c=X` a typed key, `wN`, `shot`). The presses that type text come
  from **`tools/linktest_kbd.py`, which reads `lkt_sets` out of `m_menu.c`** and models
  `M_Link_Text_Move`, so the test types on the keyboard that ships. The master goes off → master, names
  itself, sets a mixed-case passcode and the port; the member does the same plus the master's address;
  the master turns the member away ("allow list is empty"), is allowed from the list, and both report
  each other **online**; both `link.cfg` files hold exactly what was typed.
- All 27 link cases pass. `noshow` failed once in six runs (the host never saw the joiner arrive) and
  passed alone four times and in the same batch order again — noted as flaky, not explained.
- `make smoke` 5/5. The three pages captured in OpenGL (`shot`) and looked at: a truncated "TRIED TO
  CONNEC", a footer wider than the screen, the case problem above and a cursor underline that read as a
  bar over the letter below were fixed from those captures.
- **Needs a person**: the page on the real cabinets, and on the Pi's display (only the laptop's GPU was
  captured).

---

## Phases

Each phase is usable on its own, lands with its doc section rewritten from plan to record, a
README update, and the checks named. Mark does the play testing; everything that can be checked
headlessly is checked first.

### Phase 0 — prove the ground is solid (no new features)

Before writing link code, find out whether the laptop and the Pi can play one game at all. If they
cannot, everything else is built on sand.

1. **Cross-architecture determinism.** Record the baseline on the laptop
   (`tools/demotest.sh --baseline -w <dir>`), copy that work directory and the laptop's
   `legacyhome` (demos, level packs, config) to the Pi, and compare there with
   `tools/demotest.sh --home <copied legacyhome> -w <copied dir>`. The laptop is x86-64, the Pi
   ARM64 (it boots `kernel8.img`). If any demo desyncs, a networked game between them will too, and
   shared demos will play out wrong — this is the most likely showstopper and one run rules it out.
   Take it on the Pi's own demos too, in the other direction. (The Pi 3 is far slower than the
   laptop's 40 seconds; use `--quick` first.)
2. **A plain stock netgame between them.** Laptop runs a server, the Pi `connect`s by IP, one player
   each, Deathmatch then coop, on a trusted network with the security off. Checks: it connects at
   all with the arcade join screen and panel mapping in place; `Consistency()` reports no faults over
   ten minutes; the Pi 3 keeps up with the simulation; how it feels. Mark plays this.
3. **Many nodes on one machine.** Several headless instances on the laptop, each with its own copy
   of `legacyhome` and its own port, connected to one server, to see what breaks past four players
   (see scaling). No hardware needed.
4. **A Windows cabinet.** Cabinets may be Windows as well as Linux (Mark, 2026-09-13), so the same
   determinism check runs against the MSYS2 build from `tools/build.ps1` — a different compiler and
   C runtime again. The Windows binary loads but has never been played, so this is also its first
   real test. The feature must not rely on SSH or any other Linux-only tool; the tests here use SSH
   to reach the Pi only because it is convenient.

Output: a short findings section here, and a go / change-course decision.

#### Phase 0 findings (2026-09-13)

**Decision: go.** The laptop and the Pi compute the same game, and the stock netcode already plays
one game between them, including a cabinet with two local players. Nothing found needs the netcode
replaced, and Mark has played it (finding 2).

**PINNED — step 4, the Windows check, is deferred, not dropped** (Mark, 2026-09-13: his Windows box
is shut down and slow to bring up; revisit when it is running). Before Phase 3 ships at the latest,
because that is when a Windows cabinet would first play a linked game. What it needs, when it
happens: build with `build.bat`, replay the laptop's demotest baseline against that binary (same
wads — Doom 2 v1.9), and play one netgame against the Pi. It is the MinGW compiler and C runtime
that are new, not the processor.

The machines: laptop x86-64, Fedora, GCC 15; Pi 3 Model B, aarch64 Debian 13, GCC 14. Both on
Wi-Fi (192.168.1.81 and .68). Both builds use `-O3 -ffast-math` from the `Makefile`. Both
binaries from b67ef22.

1. **Determinism across the two machines: identical.**
   - The laptop's 102 record demos replayed on the Pi (`tools/demotest.sh`, laptop baseline copied
     over, run against the laptop's own IWADs): **98 identical tic for tic.** The other four were
     not simulation differences:
     - Two "ended at a different tic" with the prefix matching (`doomu_ep1_sk3_max`,
       `doomu_ep1_sk3_pacifist`). **The laptop baseline was the long one**: it carried 256 and 19
       extra no-input tics after the demo ended (momentum decaying, `fwd`/`side` zero). A lone replay
       on the laptop ends where the Pi does. This is the trailing-tic artifact `G_Synclog_Tic`
       already tries to stop, still happening when the laptop replays 8 demos at once — a harness
       flaw, not a desync. It showed up again in the reverse run, on the laptop side again.
     - One segfaulted on the Pi, intermittently: see finding 5.
   - The reverse: the Pi's own 12 record demos, baselined on the Pi and replayed on the laptop:
     **12 identical.**
   - So x86-64 against aarch64, and GCC 15 against GCC 14, with `-ffast-math`, agree. Whatever
     floating point the simulation touches is not reaching the result.
2. **A real netgame, Pi server and laptop client over Wi-Fi: identical.** A temporary probe (not
   committed) wrote every tic's random index and every player's position, angle and health on each
   node. Pi as server with two bots, laptop joined by IP: **2911 tics (83 seconds) of bots fighting
   and monsters taking damage, identical on both machines except tic 31** (finding 4). The Pi 3 kept
   up as server at full speed. The Pi hosted because the laptop's firewall was assumed to block
   inbound UDP. **It does not** (checked in Phase 3): Fedora's workstation zone allows every TCP and
   UDP port from 1025 up, so either cabinet can host.
   - **Played by Mark on real controls, 2026-09-13: "working great far as I can tell".** Pi as
     server, laptop joined, both on Wi-Fi, stock netcode with no Cabinet Link code, co-op first and
     then deathmatch. The commands, for repeating it:
     - Pi: `./doomlegacyarcade -server 2 -game doom2 -warp 1 -deathmatch -nomonsters`
     - Laptop: `./doomlegacyarcade -game doom2 -connect 192.168.1.68`
     - Leave off `-deathmatch` and the server starts co-op, which is what the first attempt did.
       Game type and rules are server netvars, so only the server's command needs them
       (`-altdeath`, `-timer`, `-nomonsters` in `D_DoomMain`). The arcade's own Deathmatch ruleset,
       `DM_both`, has no command-line switch.
3. **Many nodes, and a two-panel cabinet, on one laptop: identical.** A server, a client with
   **`localplayers 2`** and a third client with one, plus two bots: six players across three
   processes joined one game (players 3 and 4 on the two-player client, as expected), and all three
   agreed for 2400 tics except tic 31. The protocol's per-node player count works as the arcade's
   four-panel work left it.
   - Harness notes: `-server <n>` then `-connect 127.0.0.1 -clientport <port>`, one scratch
     `legacyhome` each, `addbot` from the server's `autoexec.cfg` for movement (nothing else moves
     headlessly — without bots the random index never advanced and the test proved nothing).
     Console text does not reach stdout once graphics are up and `-debugfile` is compiled out
     (`DEBUGFILE`), so `playerinfo` prints nothing; the probe used `GenPrintf(EMSG_warn, ...)`.
4. **One tic disagrees at every level start: tic 31, the client's own player's angle.** The server
   has the spawn angle (ANG90 on MAP01), the client briefly has 0, and the next tic agrees again
   because player angles are sent absolute (`EN_cmd_abs_angle`). Stock behaviour, every run, every
   client. It is harmless unless that player fires on that exact tic, and `Consistency()` does not
   include angle so it never faults. **Look at it in Phase 3**: it is the kind of thing that becomes
   a real desync once something reads the angle that tic.
5. **The Pi has a live sound crash, unrelated to networking.** `doomu-sl_E1M1_sk2_speed` segfaults
   on the Pi about **1 run in 7** (reproduced 2 of 14 headless, dummy audio driver); never on the
   laptop. Both cores are in `I_UpdateSound_sdl` on SDL's audio thread — one with channel 11's
   `leftvol_lookup` read as NULL, the other with channel 13's `rightvol_lookup` NULL — while memory
   afterwards holds valid pointers. This is the race `gotchas.md` records as fixed by ordering the
   stores. On the Pi the stores and the mixer's reads are both in the right order in the binary, so
   something else still races; **the fix is a real lock** between `I_StartSound` /
   `I_UpdateSoundParams` and the mixer, which `HAVE_MIXER` builds do not have. Separate work, but
   it matters to a Pi cabinet more than anything in this plan. Core files are on the Pi
   (`coredumpctl list`).
   - **Fixed 2026-09-13, before Phase 1**: `mix_lock` plus a mixer that works from a copy of the
     channel table. Write-up and proof in `gotchas.md` (the audio thread entry).
6. **The IWAD version decides whether scores can be shared, not the game name.** The laptop had
   Doom 2 **v1.666** (`30e3c2d0…`), the Pi **v1.9** (`25e1459c…`). Mark chose v1.9 and the laptop
   was updated (old file kept in `~/games/doom-backup/`). Replaying the laptop's 18 Doom 2 record
   demos on v1.9: **16 desync at tic 1**; the two `doom2+dwango5` ones survive because the pack
   replaces MAP01. So the per-game wad fingerprint in Phase 2 is required, not a nicety — the same
   `doom2` game ID on two IWAD versions is two different games — and the netcode's existing MD5 check
   would have refused this netgame outright.

### Phase 1 — what was built (2026-09-13)

Pairing, authentication and presence between cabinets, with nothing yet shared across the link. A
cabinet with the link switched off, or built without OpenSSL, behaves exactly as before.

**Files**
- `svn1749/src/d_link.c` / `d_link.h` — the whole feature. Named `d_link`, not `lk_link` as planned:
  the Makefile's dependency lists are grouped by filename prefix and there is no group for `l*`, so a
  header change would not have rebuilt it. `d_*` is also where the network code already lives.
- Hooks: `LK_Init` beside `HS_Init`/`AU_Init` in `D_DoomMain`, `LK_Ticker` every pass of
  `D_DoomLoop` (after `TryRunTics`), `LK_Shutdown` in `D_Quit_Save` before `D_Quit_NetGame`.
  `M_Join_Active()` added to `m_menu.c` for presence.
- `tools/linktest.sh` + `tools/linktest_peer.py` — the test, below.

**Build.** `HAVE_LINK=1` in `make_options` links `-lssl -lcrypto` and compiles `d_link.c` with
`-DHAVE_LINK`; without it `d_link.c` compiles to stubs. **Only `d_link.c` may test `HAVE_LINK`** —
every caller uses the same functions either way — and `d_link.o` depends on `make_options`, so
switching the option rebuilds one file, never leaving other objects built the other way.
`tools/build.sh` probes OpenSSL by linking (like every other library) and **appends `HAVE_LINK=1` to an
existing `make_options`** that has no `HAVE_LINK=` line — it never regenerates one, so without that the
laptop and the Pi would never have got the link. An explicit `HAVE_LINK=0` is left alone; `HAVE_LINK=1`
without OpenSSL is warned about. OpenSSL is in every distribution's `pkg_list`, and a missing OpenSSL
triggers `--install-deps`, because CI builds the release binaries that way. **`tools/build.ps1` does not
enable it yet** — Windows is pinned (step 4); `d_link.c` is POSIX sockets only until then.

**Settings** live in `legacyhome/link/link.cfg` (mode 0600, gitignored everywhere as `link/`):
`role master|member|off`, `name`, `master <host>`, `port` (default 5030), `passcode <rest of the line>`,
`allow <host>` (repeatable). Changed at the console with **`link_set <key> <value>`** in a `-devmode`
session (`link_set allow 192.168.1.68`, `link_set unallow ...`), which saves atomically and restarts
the link; **`link`** prints the status; **`link_forget`** clears the pins. The operator page is read
only for now (Arcade Options → **Cabinet Link**), so a passcode is typed at the console.

**Identity**: an ECDSA P-256 key and a self-signed certificate generated on first use
(`cabinet.key`, `cabinet.crt`, 0600 from the first byte — `fchmod` on the atomic-write temp file before
writing). The id is SHA-256 of the DER public key; the short form is its first four bytes
(`14F0-7533`). Validity is fixed at 1970–9999, because nothing checks it and a Pi with no clock boots
into 1970. `pins.txt` holds the cabinets this one has authenticated.

**The protocol**, all on one TLS 1.3 connection per member (no resumption, no tickets):
1. The master closes any connection from an address not on its allow list, or locked out, **before
   `SSL_new`** — a stranger never reaches OpenSSL. An empty list refuses everyone.
2. Mutual certificates, accepted by the verify callback; identity is decided after the handshake.
3. The member checks the master's key against its pin (if it has one) **before sending anything**.
4. Both derive `PBKDF2-HMAC-SHA256(passcode, "dla-link-v1-salt" || lower id || higher id, 60000)`.
5. The member sends `AUTH` = `HMAC(key, TLS exporter("dla-link-auth") || "member" || member id ||
   master id)`. Only if it is right does the master answer with its own (`"master"`); a wrong one
   closes the connection and counts toward a 3-strikes, 60-second lockout per address.
6. Then `HELLO` (name, build, role, panels, state), `PRESENCE` on every change, `PING` after 5 s
   quiet, dropped after 15 s silent; the master sends every member a `PEERLIST` roster when it
   changes. Frames are `u32 length, u8 type`; each type has a maximum length and anything longer,
   or of an unknown type, closes the connection. Names and build strings off the wire are sanitised
   to printable characters before anything draws them.
7. 10 seconds from accept to authenticated, or the connection is dropped. Members reconnect with
   backoff 1 s doubling to 60 s; a changed master identity goes straight to 60 s.

**Threads.** One `SDL_Thread` runs every socket and all of OpenSSL around a `poll()` loop, woken by a
pipe. The game thread and it share one struct under a mutex: this cabinet's state out, the peer
snapshot and log lines in. **The link thread never prints** — the console is not thread-safe — it
queues lines that `LK_Ticker` prints, and pin file writes are also done by the game thread. The
member's `AUTH` usually arrives in the same read as the end of its handshake, where OpenSSL has
already taken it off the socket and `poll()` will never report it, so the loop drains
`SSL_has_pending` explicitly — without that the master would sit on a proof until the timeout.

**Presence** (`lk_compute_state`): `devmode` → DEVMODE, initials page → SIGNING, join screen →
JOINING, `D_Attract_Running()` → MENU if a menu is open, otherwise IDLE; anything else is PLAYING.
HOSTING is defined for Phase 3 and not yet set.

**Status output for tests.** `link` prints to the console, and also writes `LINKSELF` / `LINKPEER`
lines, and every link log line as `LINKLOG`, with **`EMSG_errlog`** — terminal only. `EMSG_info` does
not reach stdout once graphics are up, so a headless run saw nothing at first. **`-linkstatus`** runs
`link` every 2 seconds of wall time.

**Verified**
- `tools/linktest.sh`: **11 cases, all pass** in about 2 minutes, two at a time on the laptop (`-j`; four at once was killed for memory with a browser open).
  - `pair` — master and two members: both online, one reported `devmode` and one `idle`, the second
    member sees the first through the master's roster, the ids each side reports for the other agree,
    `cabinet.key` and `pins.txt` are mode 600.
  - `passcode`, `allow`, `emptyallow`, `lockout` — refused for the right reason on both sides.
  - `identity` — a member that paired, then meets the same address and passcode with a new key,
    refuses it: `MASTER IDENTITY CHANGED`.
  - `fakemaster` — on first contact (no pin) the member does prove itself, by design, but refuses a
    master that cannot prove the passcode back. `pinnedfake` — once pinned, it sends **no** proof to a
    different key on the master's address.
  - `garbage` — random bytes are dropped, and the master still pairs a real member afterwards.
  - `bigframe` — an `AUTH` frame claiming a megabyte is dropped at once, before authenticating.
  - `unbound` — a proof with the right passcode, ids and roles but another session's exporter is
    refused.
- **`--selfcheck`: all 9 checkable cases go red** with their own check switched off, each for the
  right reason (the bad actor gets online, the proof is accepted, the frame is kept). The selfcheck
  build compiles `d_link.c` with `LK_SELFCHECK`, where `lk_selfcheck_off(name)` reads an environment
  variable; in every normal build it is the constant `false` — the installed binary contains no
  `LK_SELFCHECK` string. `pair` and `garbage` have nothing to switch off.
- An idle master costs nothing measurable: 20 seconds headless used 9.1 s of CPU with the link on
  against 11.9 s with it off (noise; the engine loop dominates).
- `make smoke` 5/5; `tools/demotest.sh` 102 compared, 0 desynced — the link changes no gameplay.
- The operator page, screenshotted in OpenGL on the real GPU with one member online and one locked
  out: a refusal reason trimmed into the status column read "REFUSED: LOCKED OU" and lost its
  meaning, so a refused cabinet's reason now gets a full-width line of its own.
- **Laptop and Pi over Wi-Fi**, built by `tools/build.sh` on the Pi's existing `make_options` (which
  appended `HAVE_LINK=1` itself): Pi as master allowing only the laptop (192.168.1.81), laptop as
  member. Both authenticated each other, each reported the other's id as that cabinet reports its own
  (Pi `7E5F-1B35`, laptop `7F00-0361`), and 14 consecutive reports showed it online and idle. With a
  wrong passcode on the laptop: the Pi refused, locked the address out after three tries, and the
  laptop said the passcodes probably differ.
- That run showed the laptop, after it quit, as **`refused: connection lost`** on the Pi — red, on
  the operator page, for a cabinet that was simply switched off. A cabinet that was authenticated and
  then leaves is now **OFFLINE**; REFUSED is only for failures before authenticating. `pair` checks
  it (members quit before the master), and the check was shown red against the build from before.

**What went wrong on the way — worth knowing before extending the test**
- **The harness reported a pass for a case that had not run.** A case function looped with `for c`,
  overwriting the variable the harness used to name the case's result file; the result was never
  found, and a missing result counted as a pass. It now needs a `finished` marker. Same lesson as
  `viewgrid-test.py`: prove the harness can fail before trusting it green.
- **Timing a status report in game tics does not work with many engines on one machine.** An
  autoexec `wait 350; link` never ran before the timeout with a dozen headless engines — the tics ran
  far behind the wall clock (`-nodraw` barely helped), and it read as the link failing. Hence
  `-linkstatus`, and four cases at a time.
- **Python's `ssl` server with `CERT_OPTIONAL` and no CA rejects a self-signed client certificate**
  and aborts the handshake, which looked like the engine refusing the fake master. `CERT_NONE` (do
  not ask) is what a stand-in master needs.
- **A master answers junk with a TLS alert before closing**, so "the peer sent something" is not "the
  connection is open": read to end-of-file.

**Not in Phase 1, on purpose**: editing settings from the menu (console only), IPv6 (IPv4 only),
discovery, Windows sockets, and a role change from master to member keeping its old pins (a member
enforces any pin it has — `link_forget` after changing role).

### Phase 2 — shared high scores and demos

- Append the new fields and the epoch to `highscores.dat` / `runs.dat`, with old files loading
  unchanged.
- The merge as **one pure function** in `hs_stuff.c`, and a test in the style of the existing
  extracted tests that drives it exhaustively for commutativity, idempotence, associativity over
  three cabinets, tie-breaking, epochs and old-format lines.
- Wad fingerprints, build and ruleset checks; demo transfer; deferred application while playing.
- Last: the cabinet tag on the attract table, if it fits (decision 3).

**Verified by:** the merge test; two headless instances with different boards converging to the same
files byte for byte; `clearhighscores` on the master spreading instead of being undone; a truncated
demo refused; a member switched off during a clear losing its old records when it reconnects;
`make demotest` still passing (the score file change must not touch the simulation).
Then Mark sets a record on the Pi and watches it appear on the laptop's attract screen.

### Phase 3 — what was built (2026-09-13)

Starting a Deathmatch or a Campaign on one cabinet invites every other cabinet that is idle (or in
its menus) and running the same game; whoever presses in there plays in the same game, over the
network, with every game packet sealed. **Built before Phase 2**, at Mark's request: invites need
only what Phase 1 built.

**Files**
- `svn1749/src/d_linkgame.c` / `d_linkgame.h` — the invite protocol and its state, entirely on the
  game thread. It does not test `HAVE_LINK`: it talks only to the `LK_*` functions, which in a
  build without OpenSSL never report a peer, so it never invites.
- `d_link.c` — transport for it (see *Protocol v2*), and the sealed game channel.
- `m_menu.c` — the join screen's side: `M_Join_Open` invites, `M_Join_Remote_Open` /
  `M_Join_Convert_To_Remote` / `M_Join_Remote_Connect` / `M_Join_Remote_Close` for the other
  cabinet's screen, remote players counted in `M_Join_Check_All_Locked`, `M_NewGame_Go` and
  `M_Arcade_MP_Go`.
- `d_clisrv.c` — `D_Link_Connect` / `D_Link_Restore_Port`. `i_tcp.c` — the seal in `SOCK_Send` and
  the open in `SOCK_Get`.

**Protocol v2** (`LK_PROTO_VERSION 2`; both cabinets need this build — a v1 cabinet fails the
proof's version byte and is refused):
- `HELLO` and `PRESENCE` carry the game id (`doom2`, `doom2+dwango5` — IWAD plus level pack, spelled
  as the score tables spell it), and the roster carries every cabinet's full id and game.
- **`ROUTE`** carries a game message to one cabinet or to all. Members only ever talk to the master,
  so the master relays — and **replaces the source a member wrote with who that member is**, so no
  cabinet can speak as another.
- The master **numbers every `INVITE`** as it passes through and tells its sender the number
  (`INVITE_ACK`). That order is the whole of how two cabinets agree who hosts.
- Game messages cross threads through two small rings under the link mutex: events in, outbox out.
  The link thread only moves bytes; every decision is on the game thread.

**The flow, as built**
1. **Host.** `M_Join_Open` for Deathmatch or Campaign asks `LKG_Would_Invite`: is some online
   cabinet running the same game and `IDLE`, `MENU` or `JOINING`? If so the join screen opens —
   **even on a one-panel cabinet**, which otherwise never shows one — and `INVITE` goes to everyone.
   A solo Campaign that nobody joins is still the scored solo run.
2. **Other cabinet.** An invite for its game, arriving while `IDLE` or `MENU`, closes any open menu
   and opens **its own join screen** for that game: the usual per-panel cells, the host's countdown,
   and a line reading `DEATHMATCH ON LAPTOP, 1 IN THERE`. Anything else (playing, signing the board,
   an operator session, another game) ignores it. Escape backs out to the attract screen.
3. **Both** send `STATUS` (panels in, all locked, seconds left) on every change and every second.
   The host's screen reads `RASPBERRYPI: 1 IN`. The game starts when every panel that pressed in,
   **on every cabinet**, has locked — or at the host's countdown.
4. **Start.** `M_Join_Start` calls `LKG_Host_Start` before the server comes up: each cabinet with
   players in gets `START` (the host's UDP port, a key id, two fresh 32-byte keys); everyone else
   gets `CANCEL`. Remote players count toward coop (`M_NewGame_Go`) and toward the server's wait
   (`M_Arcade_MP_Go`), with a **15 second timeout** so a cabinet that never arrives cannot hang the
   host. The joining cabinet hands its panels to the engine exactly as a local join does and
   connects (`D_Link_Connect`: the port goes in `server_sock_port`, because the console strips
   `:port` from `connect`).
5. **Two cabinets opening the same game at once**: a host whose join screen has no remote players
   yet, receiving an invite for the same category numbered *before* its own (or before its own has a
   number), cancels its invite and turns its screen into a join of the other — keeping who pressed in.
6. **Over.** Each side watches for leaving the network game (`netgame` false after having been in a
   level, or 30 seconds without ever getting in) and then drops its keys, puts the port and the
   download settings back.

**Differs from the plan, on purpose**
- **No banner over the attract screen**: the invited cabinet opens a real join screen at once. A
  banner needed its own drawing over the attract cycle and its own key handling, and the join screen
  already says everything — whose game, how many are in, the countdown — with the setup a player
  needs anyway. Simpler, and nothing is left to confuse.
- **The UDP port is not opened only for linked games.** Instead a cabinet with the link switched on
  **drops every game packet that does not open, linked game or not.** That turned out to matter: any
  menu-started multiplayer game — including a purely local Deathmatch — opens UDP 5029 and, before
  this, parsed whatever the network sent it.

**The game channel** (`LK_Net_Recv` / `LK_Net_Send`): `u8 key id | u32 counter | ciphertext | 16-byte
tag`, ChaCha20-Poly1305 with the 5-byte header as associated data, one key each way per joining
cabinet, fresh at every `START`. 21 bytes, so a full 1450-byte game packet still fits one Ethernet
frame. A 64-packet replay window per key. A packet that fails is dropped **before `SOCK_Get` gives
its sender a node**, and `SOCK_Get` reads the next packet at once, so junk cannot delay real traffic a
tic. The host learns which address uses which key from the first packet that opens, and drops what
it would send to an address with no key. Measured 14–25 µs per packet on the laptop.

**Downloads are off in a linked game.** `D_Link_Connect` sets `download_files` and
`download_savegame` to 0 and the end of the game restores them: a cabinet never writes a wad or a
savegame because a peer — even an authenticated one — offered it. Different wads are refused at
connect instead (the netcode's own MD5 check).

**Verified** — `tools/linktest.sh`, 19 cases at the time (23 now), all pass (two at a time, about six minutes):
- `linkgame` — master hosts a Deathmatch, member joins: host `server=1 players=2`, joiner
  `server=0 players=2`, ~1,700 packets sealed and opened each way over 55 seconds, **0 dropped**,
  both still linked at the end.
- `campaign` — the same for a Campaign: a two player coop game across the two cabinets.
- `nojoin` — nobody on the other cabinet presses in: its invite is cancelled, the host plays alone.
- `noshow` — the other cabinet joins and is killed the moment it is told to connect: the host gives
  up after the timeout and plays alone.
- `stranger` — 60 packets, random and plaintext-shaped, at the host's game port mid-game: all
  dropped, game unaffected. **Selfcheck**: with the drop switched off (only failures let through, so
  the linked game still runs) the case goes red because 0 were dropped.
- `convert` — both cabinets open a Deathmatch as soon as they see each other: one game, exactly one
  host. (In the runs so far one invite always arrived first, so this proves "never two games", not
  the numbered tie-break itself.)
- The 11 Phase 1 cases and their selfcheck, unchanged — after `tools/linktest_peer.py` learned the new
  version byte. **That mattered**: with the old byte `unbound` still passed, because the master
  rejected the version before ever checking the proof. Only the selfcheck would have shown it.
- `make smoke` 5/5; `tools/demotest.sh` 102 compared, 0 desynced.
- The two join screens, captured in OpenGL on the real GPU: the host's line and the invited
  cabinet's line are placed and sized right. Brackets were dropped from the invited line — the menu
  font draws `(` and `)` as shapes that read as other letters.
- **Laptop and Pi 3 over Wi-Fi, both directions** (`-linktest -linkautohost` on one, `-linkautojoin`
  on the other): the laptop hosting with the Pi joining, and the Pi hosting with the laptop joining.
  Each time both cabinets reached the level in one two-player game, about 2,000 packets were sealed
  and opened each way, **0 dropped**. The seal costs the Pi 3 about **100–125 µs per packet**
  (laptop 19–22 µs) — at the game's packet rate roughly a tenth of a millisecond per tic, nothing a
  player can feel.

**What went wrong on the way**
- **A joining cabinet dropped its keys 30 seconds into a live game.** It judged "the game is over" by
  `D_Attract_Running()`, which is cleared by `G_DeferedInitNew` — a route a *client* never takes — so
  a client in a game still read as sitting on its attract screen. It would also have told the other
  cabinets it was idle, and been invitable mid-game. `D_Link_Connect` now marks the attract cycle
  over, and the end of a linked game is judged by `netgame`. **The first `linkgame` case passed
  anyway** — the failure landed in the last seconds of the run — so the case now runs 55 seconds and
  fails if either side ever reports leaving the link while still in the level; the log of the failing
  run matches that check three times.
- **A Deathmatch that nobody on the other cabinet joined hung the host on "waiting for players".**
  `M_Arcade_MP_Go` set `wait_players` to the joined count, called `D_WaitPlayer_Setup`, and put the
  cvar straight back — but the `map` command then runs `SV_SpawnServer`, which calls
  `D_WaitPlayer_Setup` *again* and re-read the restored default of 2, with no timeout. That is the
  same code path as a **local one-player Deathmatch** started from the menus, which `menus.md`
  records as fixed — it very likely still hung. The restore is now queued behind the `map` command.
  This also means the 15 second no-show timeout would have been undone the same way.
- **The laptop's firewall was never the obstacle Phase 0 assumed.** Fedora's workstation zone allows
  every TCP and UDP port from 1025 up, so either cabinet can host.
- **The first real linked game refused to connect: "it uses DOOM.WAD, you are using doomu.wad".**
  Mark's laptop has Ultimate Doom as `DOOM.WAD` and the Pi as `doomu.wad` — the same file (md5
  `c4fe9fd9…`). The stock `CL_CheckFiles` (`d_netfil.c`) compared the IWAD **by name only**, which is
  wrong both ways: it refused identical files under different names, and let in *different* releases
  under the same name (Doom 2 v1.666 and v1.9 are both `DOOM2.WAD`) to play two different games. It now
  compares the md5, which `W_Load_WadFile` always computes and the server always sends; the name only
  chooses the wording of the refusal. Two cases prove it both ways and both fail on the build before:
  `iwadname` (Ultimate Doom as `doomu.wad` on one cabinet, `DOOM.WAD` on the other: one game) and
  `iwadversion` (the joiner on v1.666 against a v1.9 host: refused, the host plays alone). Getting
  `iwadversion` to test anything took two tries, both worth knowing: the engine prefers
  `~/games/doom/DOOM2.WAD` over a file linked into the cabinet's own directory, so the first run compared
  two identical files; and `-iwad` answers "File not found" for a name that does not end in `.wad`.
  - A cabinet with a different IWAD version is still *invited* (the game id is the same), and learns at
    connect; the host starts alone after its 15 second wait. Putting the IWAD's md5 into the game id
    would stop the invite instead — not done yet.
- **"Only works when initiated on the server, otherwise it times out."** Mark's cabinets, the Pi as
  master: a game started on the Pi worked, one started on the laptop showed the Pi the join screen,
  then started on the laptop alone while the Pi went back to attract. Every headless case passed,
  including new ones built to match (`memberhost`: the member hosts; `rehost`: a second game in the
  same running programs; `memberpress`: four panels, fire pressed but never locked in). Running the
  two real machines with copies of their live homes reproduced it **about one time in five** — and a
  per-packet drop reason showed the Pi sending *unsealed* 16-byte "ask info" packets every 2 s, which
  the laptop rightly threw away, while the Pi's own log read `+4294967295ms` straight after START.
  - **Cause: `LKG_Ticker` read the clock before handling its events, and the elapsed-time checks were
    plain unsigned subtractions.** START stamps `lkg_game_ms = lkg_now()` while being handled; when the
    millisecond ticked over in between, the stamp was 1 ms *newer* than `now`, `now - lkg_game_ms`
    wrapped to 49 days, the "never began in 30 s" rule fired on the spot, and the joining cabinet
    dropped its keys before it had sent a packet. The host had the mirror image: a remote's STATUS
    stamps `last_ms`, which wrapped past the 6 s "gone quiet" rule and zeroed that remote's players —
    so a host whose countdown ended in that tick started alone. A Pi 3 crosses a millisecond inside a
    tick far more often than the laptop, which is why the laptop never showed it and why it looked like
    a master/member difference: the Pi was always the one joining.
  - **Fix**: the clock is read *after* the events, and every elapsed time goes through `lkg_since()`,
    which cannot go negative. **Regression case `slowclock`**: `-linktest -linkpollsleep 3` sleeps
    between the start of the tick and its events on both cabinets, making the race certain. It **fails
    on the build before** (the host forgets the pressed-in remote and plays alone), while `memberhost`
    passes on that same build — the field symptom exactly. After the fix, **10 of 10** laptop↔Pi runs
    with copies of both live homes (four panels, attract demos, the laptop hosting, the Pi pressing in
    and locking, or pressing in only) reached one two-player game with 0 packets dropped — against 2
    failures in the 10 runs before it. All 23 link cases pass; `make smoke` 5/5.
  - The new cases stay in the suite: `memberhost`, `rehost` (`-linkendgame S`, `-linkhostafter N`),
    `memberpress` (`-linkautopress`; `LOCALPLAYERS=4`), `slowclock` (`-linkpollsleep N`), and
    `KEEPDEMOS=1` for a cabinet that keeps its record demos.
  - **Two traps on the way.** The live Pi's output goes to `~/.xsession-errors` only when stdout is
    flushed, and a test engine killed by `timeout` loses whatever was buffered — the first failing run's
    Pi log stopped mid-story. Run a cross-machine engine under `stdbuf -oL -eL`. And the laptop can run
    the suite two at a time only with memory to spare; with a browser open it was killed twice, even
    serially in the background — run it in the foreground in batches.
  - **That fix was real, and it was not what Mark was seeing.** On the fixed build he still reported
    "I start a deathmatch on the laptop, join on both laptop and Pi, and then the Pi goes back to
    attract mode". Every test so far had joined through `-linkautojoin`, which reaches into the join
    screen directly, and every scratch cabinet had the same wads — so two things the real cabinets do
    were never exercised. The Pi was relaunched with its output `tee`d to a file (it normally prints to
    a terminal, where nothing can read it back) and one attempt gave both answers.
- **The Pi was refused for two music packs.** The laptop's `legacyhome/autoexec.cfg` does
  `addfile "IDKFAv2.wad"` and `addfile "Doom2OST.wad"` — 90 MB and 328 MB, every lump a `D_` track.
  The server lists every loaded wad as needed (`Put_Server_FileNeed`), the Pi had neither, and with
  downloads off in a linked game `CL_ConnectToServer` gave up: `"IDKFAv2.wad" not found … Remove
  -nodownload`, back to attract. The laptop's `wait_timeout` then started it alone. When the *Pi*
  hosted there was nothing extra to ask for, which is the whole of "only works when initiated on the
  server". **Fix**: the host leaves out any wad whose every lump is audio (`D_` music, `DS`/`DP`
  sounds); one lump of anything else and it is required as before, and a zip archive always is.
  Music is found by name and never read by the simulation, so the two cabinets play the same game with
  their own soundtracks. Cases: `musicwad` (the host loads a 4-lump audio wad the joiner lacks: one
  game) **fails on the build before** with the field message; `gamewad` (the same wad plus one
  non-audio lump: refused, host plays alone) passes both before and after — the rule did not get loose.
  `mkwad` in `tools/linktest.sh` writes such wads.
- **An invite's join screen closed the tic it opened on a cabinet nobody had touched.**
  `G_Idle_Timeout_Check` closes any menu over the attract screen once `idletimeout` (60 s on both) has
  passed since the last *input* — and a cabinet waiting to be invited has usually had none for longer
  than that. Mark pressed the Pi's buttons before this could bite him, so it was not his symptom, but
  it would have been the next one. The join screen is now exempt, like the initials page (it has its own
  countdown). Case `idlejoin`: `idletimeout 15`, invited 20 s after boot, and the press is a **real key
  event** (`-linkpressafter 6` posts panel 1's fire through `D_PostEvent`, so it only joins if the
  screen is still up). It fails on the build before.
  - Verified: 26 link cases pass (`pair` failed once in a batch of eleven — its third engine never
    showed up, with the laptop short of memory — and passed on its own); `make smoke` 5/5.

**After the first real games** Mark reported three things (2026-09-13): link setup only from the
console; error prompts that needed Escape; and play "stuttery on the client that joins, no matter
whether the laptop or the Pi".
- **The stutter was the interpolation fraction, not the network** — see `uncapped-framerate.md`, "A
  cabinet that joined a network game must not use it". `-tictiming` measured the Pi joining at 0.40
  tic RMS error with every frame running one tic or none; counting the fraction from when the tic
  really ran brought it to 0.075, with no added delay.
- **The prompts**: a message box that pops up with no menu open (`"Server has Shutdown"`, `"Server
  Timeout"`, kicked, sync aborted) is put up by `M_StartMessage` through `M_StartControlPanel`, which
  opens the *main menu* underneath — and `M_StopMessage` stepped back to it. So fire did dismiss the
  box, onto a menu only Escape left. A message that opened the menus now closes them
  (`message_opened_menus`), ignores presses for its first half second (so mashing fire cannot skip it
  unread), and says "Press FIRE". Case `msgfire`: the host ends the game after 12 s and the joiner
  presses fire at the message through the input queue (`-linkmsgpress`); the status line's new
  `menu=` field (0 none, 1 a menu, 2 a message box) must be 0 after. On the build before it read
  `menu=1` — the main menu.

**The first eight player game** (four at each cabinet, 2026-09-13). With the laptop hosting after it
had joined the Pi's game, the Pi drew "the view in the left corner", "a slice of another player
screen", and "could only turn but not move"; the next try crashed the laptop. Two separate bugs,
neither the host/joiner order:
- **The joiner never learned its own players: an upstream packet-size bug that only eight players
  reach.** `SV_Send_Tics` splits ticcmds into sections of `NUM_SERVERTIC_CMD` (45) but packs the
  textcmds after them up to `software_MAXPACKETLENGTH`; the client's `servertic_handler` checked a
  textcmd against the end of the fixed `cmds[45]` array (360 bytes after it starts) instead of the
  packet it received. At two players a tic's ticcmds are 16 bytes and nothing comes close. At eight
  they are 64: four or five tics in a packet plus the 143 byte XD_ADDPLAYER textcmd pass 360, the
  joiner logs **`Nettics: textcmd exceed buffer`** and drops it — and so it never runs the add-player
  commands for its own four. It then sits in the level with `players=4 locals=-1,-1,-1,-1`: no player
  of its own, every view drawn from someone else's position, and turning still working because the
  view's angle comes from the panel (`localangle`). A host sends several tics per packet whenever it
  falls behind, so on the real pair it happened about one run in ten; locally, never. `endbuffer` is
  now the end of what arrived (`doomcom->datalength`), capped at `MAXPACKETLENGTH`; the check against
  the destination buffer is unchanged.
  - Found by running Mark's sequence on the two real machines (four players a side, drawing, copies of
    both live homes) until it failed, with each cabinet's local player slots added to the status line
    (`locals=`). The first theories — a join during the host's attract demo, a duplicated join
    request, the view grid — were each checked and ruled out (case `demojoin` stays as a guard).
  - **Case `slowjoin8`**: `-netbatchtics 4` makes the host send exactly four tics a packet (32
    ticcmds, 256 bytes, plus the textcmd: 399), so the old check fails **every time** (2 of 2) and the
    fix passes (2 of 2). Slowing the host's frames instead only made it likely — six tics split into a
    45 ticcmd section and a short one and fit — which is why the switch exists.
- **The laptop's crash: OpenGL dynamic lights of a freed level.** `hw_light.c` keeps one light list
  per view (`view_dynlights[4]`), and a view's planes are lit from the lights its sprites found on its
  own previous frame — mobj pointers. `HWR_SetupLevel` cleared only the list of the view drawn last,
  so views 2–4 kept pointers into the level just freed, and the first frame of the next game that lit
  a plane in one of them read a freed mobj (`HWR_PlaneLighting`, `pind=1`, from the core dump).
  `HWR_Reset_All_Lights` clears all four. **Not reproduced headlessly**: MAP01 has no light-giving
  things, idle test players fire nothing, and a freed mobj usually still reads as valid — the fix
  rests on the backtrace and the code.
- New test plumbing: `rehostview` (four a side, both drawing, both games checked; `PLAYERS_EACH=1`
  for one a side; `VIDEO=offscreen DRAWMODE=OpenGL` runs any case on the real GPU), `-linkjoinpanels`
  now presses in up to four panels, and the status line carries `views= viewport= screen= locals=`.
- Noticed, not fixed: `Send_localtextcmd` sends textcmds for local players 1 and 2 only, so a name or
  colour change from panels 3 and 4 never reaches the other machines.

**Needs a person** — not reached headlessly:
- An invite arriving while someone is in the other cabinet's **menus**, and while they are part way
  through the **guided control setup** (it should be abandoned exactly as Escape abandons it).
- Idle timeout and arcade death **in a linked game**: `G_Idle_Timeout_Check` and
  `G_Arcade_Death_Check` were written for one machine. What happens when the joining cabinet's player
  walks away, or the host's does, is untested.
- A Deathmatch against a Campaign opened at the same time (should be two separate games).
- Pulling a cable mid-game, and playing on the Pi 3 over Wi-Fi for longer than a test run.

### Phase 4 — past two cabinets

The scaling work below, done with N headless instances. Only as far as it proves worthwhile.

---

## Alternative considered: run all the game logic on the master

Mark asked (2026-09-13) whether the master could run the whole simulation, with the other cabinets
only sending input and drawing what they are told — which would make a desync impossible by
construction. It would, and it is how Quake and the client/server Doom ports (Zandronum, Odamex)
work. **It is not the plan, for three reasons:**

- **It is a new netcode, not a change to this one.** Doom Legacy, like every Doom since 1993, is
  *lockstep*: every machine runs the same simulation from the same inputs, and the server only
  collects and hands out input. Moving to a server that owns the world means sending the state of
  every monster, projectile, door and lift to every cabinet many times a second, and teaching the
  clients to draw from that instead of from their own simulation. The ports that did it spent years
  on it.
- **It costs the remote players responsiveness.** Their own movement would wait for a round trip to
  the master unless client-side prediction were built as well — a second large piece, and exactly the
  input latency this cabinet refuses to add.
- **It would not remove the need for determinism anyway.** A record demo *is* a list of inputs. A
  record set on the Pi and shared to the laptop has to replay identically on the laptop, whoever ran
  the game, so the laptop and the Pi must compute the same simulation regardless. Sharing scores
  requires the very property this alternative was meant to avoid depending on.

Lockstep also comes with a safety net: `Consistency()` compares every node every tic and the server
repairs a node that has drifted (`SV_consistency_fault`, and a savegame resend). So the plan keeps
lockstep, and Phase 0 checks the one thing it depends on. If the laptop and the Pi turn out not to
agree, the fix is to find the non-deterministic code — which shared demos need fixed in any case.

---

## Scaling past two cabinets

Cheap, and designed in from the start: the star link, presence, invite fan-out, the merge (that is
what associativity is for), the per-node player counts, `MAXNETNODES 32`.

Not free, and **not claimed to work until each is checked**:

- **Player starts.** Doom maps have 4 coop starts. Legacy has a coop spawn path for extra players
  (`g_game.c` ~2825) and deathmatch keeps up to `MAX_DM_STARTS 64`, but what player 9 in a coop
  campaign actually does needs looking at.
- **Intermission and HUD.** `wi_stuff.c` sizes its arrays at `MAXPLAYERS`, but a table laid out on a
  320x200 page with 16 or 32 rows is a layout problem — measure against the real font, per the
  layout rule.
- **Player colours.** There are far fewer colours than 32 players.
- **Packet size.** A servertic packet carries a ticcmd per player inside `MAXPACKETLENGTH 1450`,
  less the 24 bytes the shim adds. Check the arithmetic at 32 players.
- **The Pi as host.** Every node runs the full simulation, but the host also does the server's work.
  A Pi 3 hosting eight players may not keep up; the laptop hosting may be the rule.

---

## Decisions

Answered by Mark on 2026-09-13; the sections above already reflect them.

1. **Passcode is sufficient.** No password-authenticated key exchange. Traffic is additionally
   limited to configured addresses, and on an untrusted Wi-Fi network the cabinets go on wired
   Ethernet with fixed addresses. → *Address allow list*
2. **Campaign invites other cabinets.** → *Which games invite*
3. **Which cabinet set a record: a good idea, not essential.** The cabinet ID is stored with every
   record regardless (sync needs it for tie-breaking). Showing it — a small tag on the attract table
   such as `MLR · PI` — is the **last item of Phase 2**, and is dropped if the table has no room for
   it at its widest glyphs.
4. **Clearing scores on the master clears every cabinet.** Proven by the headless test, not by hand.
   → *File format changes*
5. **An invite interrupts the menus**, so people setting up a networked game on two cabinets are not
   left confused; a cabinet already on its own join screen for the same game joins the earlier one.
   → *Cabinet state* and *During and after*
6. **Master off: members keep full function on their own, sync later.** Assumed throughout.
