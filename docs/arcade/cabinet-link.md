# Cabinet Link: networked cabinets (Phases 1, 2 and 3 built; 4 still a plan)

*Part of the DoomLegacy arcade cabinet build. Read before touching `d_link.c`/`d_link.h`,
`d_linkgame.c`, `d_linkscore.c`, `d_linksel.c`, `hs_merge.c`, `tools/linktest.sh`, the `HAVE_LINK`
build option, or `i_tcp.c` socket code. Written 2026-09-13 as a plan before any code; each phase's
section is replaced with the record of what was built, what broke and how it was verified as it
lands. **Phase 1 (identity, pairing, presence), Phase 2 (shared high scores and demos) and Phase 3
(invites and linked games) are built**, and Select Game Sync after them — see the "what was built"
section of each. The design sections were written
first; where the build went another way, the Phase 2 record says so and the design text is marked.*

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

- `highscores.dat` gains `set_time cabinet_id`. **Built differently:** the demo's SHA-256 is not
  stored but computed from the file whenever a manifest is built (cached by size and modification
  time), so it cannot go stale against the demo. `set_time` is Unix seconds from the cabinet that set
  it. Old lines read back with `set_time 0` (oldest) and the cabinet **unknown** — not the local
  cabinet's id, which turned the same old record held by two cabinets into two entries.
- `runs.dat` gains `set_time cabinet_id`. The run board is a union of both cabinets' entries,
  deduplicated on the whole tuple, sorted by the existing `HS_Board_Sort` rules, trimmed to the
  board's size.
- A **board epoch** in a new header line. `clearhighscores` on the master sets it — **built as the
  time of the clear**, not a counter, so a cabinet that missed the clear drops only records set before
  it. **Without it a cleared board comes straight back from the other cabinet on the next sync**; with
  it, entries from before the clear are discarded and the clear spreads instead. Clearing is allowed on the master only, so two
  cabinets cannot clear at once and race. **Clearing on the master clears every cabinet** (decided
  2026-09-13) — which is also why a stranger on the network must not be able to send one.
  - Nobody needs to try this by hand on the real cabinets: it is exactly the case the two-instance
    headless test in Phase 2 covers, against scratch copies of `legacyhome`, and that test is where
    it gets proven. A member that was switched off during the clear is part of that test too — it
    must lose its old records when it reconnects, not resurrect them.
- Clock sanity: a Pi without a real-time clock boots in 1970 until NTP answers. A `set_time` before
  2026-01-01 is stored as unknown (`0`) rather than as a real time. **Built:** `0` ranks as the
  *oldest* on a tie, which is the rule a cabinet always applied locally (the first to reach a time
  keeps its place) — an exact tic tie between an old record and a new one is rare enough that
  consistency with the local board won. A master with no clock refuses to clear.

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
  handshake hashes the ones it does not. **Built** as the settings a record demo's header carries that
  the ruleset does not pin (rocket trails, view height, invulnerability sky), checked against
  `G_BeginRecording` by `tools/hsmerge-test.py`. A mismatch is a warning on the operator page and
  pauses score sync, because a record under different rules is a different board.
- **Built:** only the game the cabinet is *running* is shared (its id and its `-sl` twin). Records for
  other games stay local until both cabinets run that game.

### Demo transfer

- Demos travel with their `highscores.dat` entry: when the merge picks a remote record, the local
  cabinet requests that demo by SHA-256.
- Hash checked, header checked, then written in place on the main thread. **Built:** held in memory
  until the merge is applied and written with `M_Atomic_Write_Open`/`Close`, rather than parked on disk
  as `.tmp` files — an apply that never happens leaves nothing behind.
- **On Windows a rename over an open file fails**, and the attract cycle may be playing that very
  demo. The main thread retries the rename after the demo ends rather than failing the sync.
- The attract cycle needs no change: it already builds its demo list from the score table.

### When sync runs

On connect, then whenever a board changes (every score file write changes `HS_Sync_Generation`), and
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
  through the input queue (`-linktest -linkkeys "<tokens>"`: `open`, `arcade` (Arcade Options), `u d l r f b` for panel 1's
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
that are new, not the processor. **The code side is done** (2026-09-15, *Windows port* below): the
link now builds for Windows, so this check can include a linked game as well. The check itself is
still pinned.

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
triggers `--install-deps`, because CI builds the release binaries that way. `tools/build.ps1` does the
same on Windows since 2026-09-15 — see *Windows port* below.

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

### Phase 2 — what was built (2026-09-14)

Linked cabinets hold one set of high scores. A record set on either appears on the other with its
demo, a cabinet that was switched off catches up when it comes back, and clearing the scores on the
master clears them everywhere. A cabinet with the link off plays and scores exactly as before, on the
new file format.

**Files**
- `svn1749/src/hs_merge.c` / `hs_merge.h` — the merge, with **no engine includes**, so
  `tools/hsmerge-test.py` compiles the real file. It also owns the board ranking
  (`HSM_Run_Rank_Cmp`, `HSM_Map_Order`, `HSM_Same_Board`): `hs_stuff.c`'s `HS_Run_Cmp` and friends
  call it, so a local insert and a merge can never rank differently. `hs_run_t` **is** `hsm_run_t`.
- `svn1749/src/d_linkscore.c` / `.h` — the sync, on the game thread, called from `LK_Ticker` right
  after `LKG_Ticker`. `hs_stuff.c` keeps ownership of the tables and files; the sync only calls
  `HS_Sync_Export` / `HS_Sync_Import` / `HS_Sync_Busy` and the demo path helpers.
- `d_link.c` — a `SYNC` frame (between a member and its master only, never relayed),
  `LK_Sync_Send` / `LK_Sync_Poll` / `LK_Sync_Peers`, `LK_Sha256`, `LK_Build`. **`LK_PROTO_VERSION` is
  3**, so an older build is refused at `AUTH` rather than closing on an unknown frame.
- `d_netfil.c` — `D_Net_Wad_Md5s`: the md5s `Put_Server_FileNeed` sends (soundtrack-only wads left
  out), for the wad fingerprint.

**File formats** (`hs_stuff.c`). Both files append `set_time cabinet` and carry `# epoch N` in their
header — a comment line, which older builds skip. Old six and seven field lines load unchanged, as set
time 0 and cabinet unknown (written `-`). Both files are now written in **canonical order**
(`HSM_Normalize`: rows by game and map order, each board in rank order), so two cabinets holding the
same scores hold the same bytes; nothing read the old order. `HS_MAX_MAPS` 64 → 256 and
`HS_MAX_RUNS` 256 → 1024, because a merged table is the union (the laptop alone used 37 rows of 64).
- **The first save under this build trims every board to its depth.** The old code only trimmed the
  board it was inserting into, and the laptop's `runs.dat` still held entries below one deep Survival
  boards — `doomu E1M1 E1M8 0 speed 14574` under `14330`, `doomu E2M1 E2M1 0 speed 1601` under a
  completed E2M8 — which no page ever showed. Checked by scoring a level against copies of the live
  files: every other line of both survived the rewrite.
- `HS_Run_As_Entry` (the run in progress, as compared against its board) now gets the current time
  and cabinet, as it will when committed. At set time 0 it counted as the oldest entry and would win a
  tie it then loses at commit, and the "leading" demo snapshot would belong to a run that placed
  second.
- `clearhighscores` is refused on a **member** ("clear them on the master"). On the **master** the
  epoch becomes the time of the clear, refused if the clock is not set. The files are written empty,
  carrying the epoch, instead of deleted — a deleted file forgets the epoch.

**The merge** (`HSM_Merge`). Split records keep the best per `(game, map, category, skill)`; boards
keep the union, each entry once, trimmed to depth; an entry both sides hold keeps whichever initials
were entered. Every order is total: tics, then set time (0 = oldest), then cabinet id, then the rest
of the record. Two decisions differ from the plan above:
- **The epoch is the time of the clear, not a counter.** With a counter, a cabinet switched off during
  a clear — or one that joins the group later — loses *everything*, including records it set after
  the clear. As a time, a set behind on epochs keeps its records set at or after it. The price: the
  merge is associative only among sets on one epoch, because merging two sets trims a board, which can
  drop a newer, slower entry that a later clear would have left standing. That only bites on the first
  sync after a clear (from then on every cabinet holds the new epoch), and members sync only with the
  master, so all cabinets still converge. The test checks exactly that: full associativity on one
  epoch, and "a merge across epochs is both sides filtered to the new epoch, then merged there".
- **An old record's cabinet stays unknown.** The first version filled in each side's own id before
  merging, so two cabinets holding the same pre-Phase-2 record (a copied `runs.dat`, say) held two
  entries, and a three deep single level board would show one run twice and push a real one off.
  Caught while designing the byte-for-byte test, before it ran.

**The protocol** (`d_linkscore.c`, little-endian, over `LK_Sync_Send`):
- `OFFER` (the manifest's SHA-256 and length) when this cabinet's manifest changes, when a peer comes
  online, and every 10 minutes. A member syncs with its master and a master with every member, so
  everything meets at the master.
- The receiver **pulls**: `GET` a manifest or demo by hash, from an offset, four chunks of about 4 KB;
  the owner answers `DATA` or `NONE`. Nothing is sent that was not asked for, so a link connection's
  16 KB buffer never fills (a full one closes the connection), and the link thread moves a chunk into
  a connection only with 8 KB to spare, so presence and invites always fit. A quiet transfer is asked
  for again after 4 s, six times; then a manifest retries after 30 s, and a demo is given up with its
  record while the merge goes ahead without it. The laptop's whole Ultimate Doom history, 77 demos,
  moved in about 4 s.
- **The manifest** is text: `build`, `game <id> <wad fingerprint>`, `rules <hash>`, `epoch`, then one
  line per record, for the running game id and its `-sl` twin only. A record goes out only with its
  demo (split records and Survival entries); single level board entries have no demo of their own.
  The receiver treats every field as hostile: the game id must be the running game's, map names
  `MAPnn` or `ExMy`, categories and skills in range, cabinet ids and initials from a fixed alphabet —
  game ids and map names become demo file names.
- **Comparable only when** the build (the `git describe` string, so the commit), the wad fingerprint
  (SHA-256 over the md5s) and the rules hash all match. A mismatch shares no records but **still takes
  the epoch**, so a clear reaches a cabinet running another game. The operator page says why under
  that cabinet, in red (`SCORES: DIFFERENT BUILD` / `DIFFERENT WADS` / `DIFFERENT SETTINGS`,
  `SCORES: PLAYING <game>`), and nothing when all is well.
- **A received demo** must match its hash, start with the DoomLegacy demo header and end with the
  demo end marker (`demo_looks_whole`; all 102 of the laptop's pass), or it is refused with its record.
- **Applied only** when `HS_Sync_Busy` is false (no run being scored, no death demo pending, no
  initials waiting) and the cabinet is on the attract screen or in its menus — not in a game, not
  signing, not in an operator session. The merge is re-planned against the local scores at that
  moment, since they may have moved on. When the epoch moved forward, every record demo the merge no
  longer references is deleted, as `clearhighscores` does on the master.

**Verified**
- `tools/hsmerge-test.py`: the laws on 3000 random rounds drawn from a tiny domain (two tic counts,
  three cabinets, epochs between the set times), pinned cases for each tie rule, and the demo-header
  rules check. **`--selfcheck`: 12 of 12 breaks go red.** One break first came back green: its
  replacement text changed nothing, which is what the selfcheck exists to catch.
- `tools/linktest.sh`, new cases — and four of them run against a build with its protection switched
  off (applying during a game, accepting a cut-short demo, keeping a cleared record's demo, ignoring
  different settings), each failing for exactly its own reason:
  - `scores` — old format against new, each cabinet with records and demos the other lacks: both end
    with **byte-identical** `highscores.dat` and `runs.dat`, the faster MAP01 and the further (slower)
    Survival run won, and every record's demo on both is the one that set it.
  - `scoreclear` — the master clears while the member is off; the member comes back, loses its old
    record and demo, and its record from after the clear reaches the master.
  - `scoreclearlive` — synced, then cleared: the member's boards and demos go.
  - `scorebad` — a demo cut short is refused with its record; the rest merges.
  - `scorerules` — rocket trails off on one cabinet: nothing shared, and the status says why.
  - `scorebusy` — the member is in a level: the merge waits in `apply`, then lands after it leaves.
  - `scoreslarge` — `SCOREHOME=<a legacyhome> GAME=doomu`: a real cabinet's history onto an empty one
    (skipped without `SCOREHOME`). With the laptop's live files: 77 demos, 62 records, 89 board
    entries, all identical.
  - `-linkcmdat S "text"` types console text S seconds after start, in any state.
    `tools/linktest-demos/` holds the six small real record demos the score cases use.
- 17 existing link cases pass; `make smoke` 5/5; `make demotest` the same 16 Doom 2 demos desynced as
  before (the v1.9 swap) and nothing new.
- The operator page with a settings mismatch, captured in OpenGL: the red line sits under that
  cabinet and fits.

**Not done**
- **The cabinet tag on the attract table** (decision 3). Records store the short cabinet id; showing
  a name needs the names of cabinets that may be switched off (the pins keep them), and the table has
  to be re-measured at its widest glyphs. Left for Mark to decide.
- On Windows a demo that is playing cannot be replaced (`M_Atomic_Write_Close`'s rename fails).
  Windows is pinned.
- Pre-existing, now more visible: a Survival demo is snapshotted while a run *leads*, so a run that led
  at a level exit and was then voided (a cheat) leaves its demo in the file with no board entry, and
  the sync pairs that demo with the board's real entry.

**Needs a person**: set a record on the Pi and watch it appear on the laptop's attract screen, then
the other way round; clear on the master and watch the member's boards empty.

#### Undoing a clear: backups and `restorehighscores` (2026-09-14)

**What happened first.** The morning Phase 2 shipped, Mark cleared on the laptop (the master) and then
put a backup `legacyhome` back — and the scores stayed cleared. Reconstructed from file times: the
laptop cleared at 08:08:08 and the Pi took the clear at 08:08:55; the backup copied back had been
taken at 08:10:44, *after* the clear (the pre-clear copy, taken at 08:07, had landed inside
`legacyhome` rather than beside it). And even a good copy on the laptop alone would have been cleared
again: the Pi still held the clear time, so at the next sync the laptop's restored records — all set
before it — would be dropped. Restoring both cabinets' own backups with both programs off recovered
everything. The design was working as written; it just made "put the old files back" a trap.

**Built**
- **`clearhighscores` keeps a copy first**: `legacyhome/scores-backup/<YYYYMMDD-HHMMSS>/` holds
  `highscores.dat`, `runs.dat` and every `demos/*.lmp`, each copied atomically. If any copy fails the
  backup is removed and **nothing is cleared**. Nothing to keep (no records, no demos) makes no backup.
  The newest ten are kept; pruning removes only the files the backup code writes, so a folder with
  anything else in it stays. On a master the epoch now moves only after the backup succeeds.
- **`restorehighscores [folder]`** (console) and **`-restorehighscores [folder]`** (startup, after
  `-clearhighscores`): no argument restores the newest backup, a name picks a backup, anything else is a
  folder holding `highscores.dat` and/or `runs.dat` and optionally `demos/` — an old `legacyhome`.
  - The backup is read by **the same parser as the live files** (`HS_Read_Splits_File` /
    `HS_Read_Runs_File`, which `HS_Load` and `HS_Runs_Load` now call), so old formats restore exactly
    as they would load.
  - It is **merged** with the current scores (`HSM_Merge`), not copied over them, so a record set since
    the clear is kept, and **both sides are put under the current epoch** (or the backup's, if later).
    That is the whole mechanism: the restored records count as held under the clear, so the member —
    also at that epoch — takes them in a plain union, instead of the master's own file falling behind
    and the member clearing it again.
  - **Demos**: a merged record that is still this cabinet's own keeps its demo; one that came from the
    backup gets the backup's demo copied in (atomically, binary), or — when the backup has none — the
    replaced record's demo is deleted so it is not shown as the new record's run. Survival boards the
    same way, by their top entry. Then `HS_Sync_Import`, which saves and lets the sync offer the result.
  - Refused on a **member** ("restore them on the master") and **during a game** or with a run,
    death demo or initials pending.
- `M_Atomic_Write_Open_Binary` (`m_misc.c`): `M_Atomic_Write_Open` is text mode, which on Windows would
  turn every `0x0A` in a demo into `0x0D 0x0A`. The sync's demo writes (`d_linkscore.c`) use it too now.
- `-linkcmdat` now takes up to four occurrences, so one cabinet can clear and then restore.

**Verified**
- `scorerestore`: synced cabinets; the master clears (backup of 3 demos made), the member empties;
  `restorehighscores` on the master brings back 2 records, 3 board entries and 3 demos, the member
  receives all three demos, both files identical and still carrying the clear's epoch. The member's own
  `restorehighscores` is refused.
- `scorerestorepath`: both cabinets already cleared (epoch in their files); an old-format folder
  restored by path on the master reaches the member.
- **Both shown red** with the restore taking the backup's epoch instead of keeping the clear's: the
  restored records never reached the member (and in the second case the two files ended at different
  epochs).
- Pruning, standalone: eleven old backups plus a clear kept the newest ten; the oldest, holding a file
  the backup code does not write, was emptied of its own files and left standing.
- All eight other score cases (`scoreslarge` with the laptop's live files: 79 demos, 63 records, 92
  board entries), `pair`, `linkgame`, `names8`, `tools/hsmerge-test.py`, `make smoke` 5/5.

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
   players in gets `START` (the host's UDP port, a key id, two fresh 32-byte keys, and the host's
   idle timeout and idle warning — see "Shared timeouts" below); everyone else
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
- **Shared timeouts: every cabinet in a linked game runs on the host's** (2026-09-14). Mark: "idle
  timeout, idle warning, and join screen timeout should all be shared on a cabinet link, otherwise we
  might get some weird behavior". Two of the three needed work:
  - **The join screen countdown already was.** `INVITE` carries the host's seconds left and `STATUS`
    keeps resetting a remote's countdown to them (`M_Join_Set_Countdown`), so a remote's own
    `jointime` is never consulted for the host's game. Note the host's own **Off** means no join
    screen, and so no invite: a cabinet set to Off never starts a linked game (`M_Join_Open`).
  - **The idle timeout and warning were each cabinet's own.** All cabinets already agreed on *how
    long* the game had been idle (`game_input_tic`, from the ticcmds), but each compared that with its
    own `idletimeout`: a joiner set shorter than the host dropped out of a game the host was still
    running and went back to attract, and each screen counted its warning down from its own number.
    `START` now carries the host's two values (`LKG_START_IDLE`, u16 each; a `-devmode` host sends
    timeout 0, since it never times out), and `G_Idle_Timeout_Check` takes them through
    `LKG_Host_Idle_Settings` while the cabinet is `LKGM_GAME_CLIENT` in a netgame — **before** its Off
    test, so a joiner set to Off still leaves a game whose host is not.
  - **Not netvars.** `CV_NETVAR` is the engine's way to share a setting, but a netvar received by a
    client overwrites the cvar itself, and only demo playback (`CV_Restore_User_Settings`) puts user
    values back — a joining cabinet would have kept the host's idle timeout for its own games after
    the linked one. `idletimeout` also exceeds a byte, which a plain netvar stores only in `.EV`. A
    value held by the link and consulted only during the game needs nothing restored. `START` grew
    four bytes, so both cabinets need this build — already a requirement (`DLA_VERSION`).
  - Settings changed on the host *during* a game are not re-sent. Only a `-devmode` session can change
    them, and the idle timeout does not run in one.
  - Case `idlehost`: host `idletimeout 900`, joiner `15`, nobody playing; the joiner must log the
    host's values and still be in the game at the end. **Fails on the build before** — the joiner left
    at 15 s (`netgame=0`, back on its attract screen). `idleshared`, `idleall` still pass.
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

**…and then the joiner was kicked** (same day). With those fixed, Mark: "only being able to turn left
and right (player 1), and then got booted out of the game in a matter of seconds". The Pi's terminal
showed a run of `Client repair` / `Client player_repair` lines (positions and momentum different,
ammo 31 against 32, armour 64 against 0) and then the game ended: a **consistency failure**. The server
drops a client's ticcmds for a tic whose consistency does not match — so the player stops moving while
its view still turns from `localangle` — and kicks the node after a few.
- **Reproduced**: first on the real pair (four a side, everyone walking with the reworked
  `-linkmoveevery`, which now holds forward and turn on every joined panel; `Consistency failure (
  server=52C client=52B ), msg tic 78, Kick node 1`), then locally on demand with **`-linktest
  -linknetloss P`**, which throws away P percent of the linked game's packets in `LK_Net_Recv` (its
  own random numbers). At 30% the build before was kicked in 4 runs of 4.
- **Found** with a temporary per-tic ring (a hash of every player's ticcmd, the textcmd, consistency
  and the random index, kept in memory and written out only at the first fault — writing each tic to
  the terminal changed the Pi's timing enough to hide the failure in eight real-pair runs). The first
  difference was a tic whose ticcmds, on the joiner, hashed to exactly **eight all-zero ticcmds**: it
  ran tics whose ticcmds had never arrived.
- **Cause — upstream, and invisible at two players.** `servertic_handler` recognised a packet's
  sections by `btic_hash(start_tic)`, which is `start_tic >> 4`: every packet starting in the same
  sixteen tics was "the same packet", so the section bits of two different packets (a resend with a
  different length, a different split) added up to 0xFF, "all received", for tics whose ticcmds were in
  a section that never arrived. With two players a packet is one section that marks itself complete;
  eight players and a lossy link split them all the time. It also cleared, on a new packet, tics that
  were already complete but not yet run, and judged readiness on the packet's first tic, which could be
  one of those. Now: a packet is its start tic, tic count and player mask (`start_tic_hash`, 32 bits);
  tics before `cl_need_tic` are never cleared, marked or written; readiness is judged on `cl_need_tic`.
  The copy loop also now counts `num_cmds` per ticcmd (it counted per tic, so a section ending part way
  through a tic read the rest of that tic from past its ticcmds).
- **And four players a node, not two**, in the server code the kick exposed: a consistency kick and a
  quit or timeout removed only players 1 and 2 of a node (the other two stayed in the game, unplayed —
  `players=6` after the kick); `SV_Maketic` filled a missed tic from player 2's slot for players 2–4
  and never for 3 and 4; `SV_Reset_NetNode` cleared slots 1 and 2 only.
- **Verified**: `lossy8` (move8 at 30% loss; checks what a player sees — still in, still walking) passes,
  and fails on the build before (dropped out of the game). At 30% loss the fixed build ran 12 times with
  no kick; two runs had a single consistency repair the server's player repair healed (a joiner's player
  missing its ticcmd for one tic, still unexplained at that loss rate). `move8` (no loss), `slowjoin8`,
  `rehostview`, `joinview`, `linkgame`, `campaign`, `nojoin`, `noshow`, `msgfire`, `memberpress`,
  `idleshared`, `demojoin`, `stranger`, `iwadname`, `pair` pass; `make smoke` 5/5; `make demotest` the
  same 16 desynced demos (by name) as the build before. On the real pair (laptop hosting, Pi 3
  joining over Wi-Fi, four a side, everyone walking) two runs had no repair and no kick, the Pi in the
  game with its own four players to the end.

**…and still kicked with real players** (same evening, on that build). Mark's Pi log: a run of
`Client player_repair` lines where nearly every player's **angle** differed by a little, then the
kick — and some `Client repair` lines with nonsense (`gametic client 11026 server 266998803`, random
state `(00600000,00010000)`).
- **The tests could not see it.** `-linkmoveevery` holds the same buttons for 400 ms, so a tic run with
  its neighbour's ticcmds — or with none, where the player was not moving anyway — looked exactly like
  the right one. **`-linkchaos`** now gives every joined panel a new random mix of forward, back, turns,
  strafe and fire every 50 ms, like people on analog sticks, and case **`chaos8`** (four a side,
  `LOSS8` percent loss, default 10) fails on any consistency failure or repair at all. With it the
  desync reproduced **on one machine with no packet loss**.
- **Cause (upstream): a tic ran on the server with ticcmds its packet did not carry.** A packet carries
  ticcmds only for the players in `ticcmd_player_mask` when it is sent, and the server makes and sends
  tics ahead of running them — so the tics made in the moment between a joining cabinet's players
  being added and the server running that XD_ADDPLAYER went out without their ticcmds, while the
  server had already stored them (`client_cmd_handler` writes them as they arrive) and ran the tics with
  them. The tic log showed it exactly: at the tic in question the joining cabinet's first player was
  strafing on the host and still on the joiner, every other player identical. `SV_Maketic` now gives
  every player not in the mask a zero ticcmd when the tic is made, so the server runs what it sends;
  and the client zeroes a tic's ticcmds (not just their flags) when a new packet first fills it, so a
  player a packet carries nothing for runs with zero rather than whatever the ring slot held.
- **Verified**: `chaos8` with no loss failed in 2 of 3 runs on the build without this and passed in
  every run with it; two cabinets' tic logs agreed for 1400 tics, every player's ticcmd included. At
  10% loss 13 of 14 runs passed and at 30% 2 of 2; the one failure at 10% left no log and did not recur
  in 13 logged or unlogged runs after it — unexplained. On the real pair with `-linkchaos` (laptop
  hosting, Pi joining over Wi-Fi, four a side) a run had no repair and no kick; a second was cut short
  by the laptop running out of memory. `chaos8 lossy8 move8 slowjoin8 rehostview linkgame msgfire
  noshow campaign` pass, `make smoke` 5/5.
- Also fixed on the way: `SV_Send_player_desc`, splitting a repair of all players over two packets,
  kept writing on from where the first ended, so the second re-sent the first players and could run past
  netbuffer. **Not found**: where the nonsense repair headers come from — they only appear during a
  repair, which the desync fix above should now make rare.

**…then a campaign desynced right after a clean deathmatch** (same build, eight players). Mark's
laptop log, joining: `Client repair: gametic 9336, update client P_random index 157 to server 158`,
more repairs, then game over. The random index one out with every ticcmd agreeing means one cabinet
drew a `P_Random` the other did not.
- **Found by logging every `P_Random` caller.** A temporary ring recorded the tic, the index and
  `__builtin_return_address(0)` relative to `P_Random` (marked `noinline`) on both cabinets, dumped
  30 tics after the first fault; `addr2line` on the first caller that differed named it. `chaos8` with
  `CAT8=campaign` (new) reproduced it with no loss in 2 of 4 runs, while the deathmatch case never had.
  The logs agreed to the call until **`A_SmokeTrailer`** (`p_fab.c`), drawn in tic 201 on the joiner
  and tic 202 on the host.
- **Cause: `game_comp_tic` was never shared.** The rocket and lost soul trail (and the revenant tracer,
  `A_Tracer`) puff only when `game_comp_tic % 4 == 0`, and each puff draws a `P_Random`. That counter
  was made for demos (it goes into the demo header) and counts from program start; a joining cabinet
  took the server's `gametic` in `PT_SERVERCFG` but kept its own `game_comp_tic`, so the two phases
  matched only by luck. The first trail after that is a desync. The deathmatch test never fired
  anything with a trail, which is why only campaign showed it.
- **Fix**: `PT_SERVERCFG` carries `game_comp_tic` with `gametic` and the client takes both;
  `random_state_t` carries it too, so the state, wait and repair messages that already reset the random
  indexes put it right as well (and say so in the log). **`NETWORK_VERSION` is 27**: the packets changed
  shape, so a cabinet on an older build is refused at join rather than misreading them — both cabinets
  need this build.
- **Test that fails every time, not only when a rocket flies**: `game_comp_tic - gametic` is constant
  through a game, so the status line prints it as `trail=` and `same_trail` (in `campaign` and
  `chaos8`) requires host and joiner to print the same. With the two assignments disabled `campaign`
  failed (`host trail=-62 joiner trail=-69`); with them both cabinets printed `trail=-63` on every line.
  `chaos8 CAT8=campaign` passed 6 of 6 runs with the tic logs identical for 1400 tics (trails were
  actually drawn in one; the `same_trail` check is what covers the rest). `campaign linkgame chaos8`
  (both kinds) `move8 rehostview iwadversion musicwad memberhost` pass, `make smoke` 5/5.
- Mark played it: fixed.

**A joining cabinet's players 3 and 4 were "player 7" and "player 8"** on the intermission (Mark,
eight players). Their names, colours, weapon preferences and artifact uses never left the cabinet.
- **Cause (upstream, two players assumed):** a client's text commands queue per local player in
  `localtextcmd[pind]`, and `Send_localtextcmd` sent and cleared `[0]` and `[1]` only, so `[2]` and
  `[3]` filled and sat there. The server's `net_textcmd_handler` would not have taken them either: it
  dropped any `PT_TEXTCMD` with more than 3 items — the whole packet, panels 1 and 2 included — and
  bounded the decode at two items. `D_Send_PlayerConfig`, which re-announces a node's players when
  someone joins, covered pind 0 and a splitscreen pind 1 only.
- **Fix**: all three loop over `MAXSPLITSCREENPLAYERS`. **`NETWORK_VERSION` is 28**, since a 27 host
  drops a four-item packet whole. On one cabinet this also means panels 3 and 4's weapon preference and
  autoaim now reach their players (they were sent through the same buffers); `demotest` shows no new
  desync — the 16 Doom 2 demos already known to desync, and three Ultimate Doom demos that end at a
  different tic than the old baseline on the build before this one too, with no desync.
- **Test**: `names8` — four a side, every panel its own name and colour (`setnames`), and the joining
  cabinet renames panel 4 and recolours panel 3 15 s into the level with the new **`-linkcmdafter S
  "text"`** (console text S seconds of wall time into a linked level; a tic `wait` runs far behind in
  the harness). (`LINKNAMES` has since gained a leading `teamplay=N teamdamage=N`, the team rules
as the game is playing them; see Team Deathmatch below.) The status now prints **`LINKNAMES`**, every player in the game as `pn=name/colour`,
  and both cabinets must end with the same eight including the rename. Without the fix both listed
  `6=Player 7/0 7=Player 8/0`; with it `6=JB3/9 7=JB4NEW/8`. `names8 chaos8 campaign linkgame move8
  joinview msgfire menusetup iwadversion memberhost` pass, `make smoke` 5/5.
- Not changed: `Send_WeaponPref_pind` still reads `cv_weaponpref[pind]` rather than the panel's, unlike
  names and colours (`D_Panel_Of`), so with panels joining out of order a player gets another panel's
  weapon order and autoaim.

**An invited cabinet nobody pressed in on "froze at 0 seconds"** (Mark). It had gone nowhere: the page
had closed, but nothing was running underneath it.
- **Cause:** an invite that arrives during an attract **record demo** stops the demo on the way to
  opening the join screen, leaving `gamestate` 0 (GS_NULL). When the invite ended (`LK_GM_CANCEL`, the
  grace timeout, or a START with nobody in) `M_Join_Remote_Close` only cleared the menus — no
  `D_StartTitle`, which is what backing out of any menu on the attract screen does — so the cabinet sat
  in GS_NULL indefinitely with the join screen's last frame, countdown at 0, still on the panel. An invite
  that lands on a title *page* has nothing stopped, the page cycle carries on, and that is the only way
  any test had ever delivered one: `nojoin` passed throughout, and its end state was never checked.
- **Found** with a temporary once-per-2-seconds print of the loop state in `TryRunTics`: tics ran
  normally and the invite closed, but `gs=0 demo_ctrl=2 demoplay=0` for good afterwards. (Not the
  `trail=` value, which stops moving whenever no gameplay tic runs, on a title page too — that briefly
  looked like a stopped clock.)
- **Fix**: `M_Join_Remote_Back_To_Attract` — `D_StartTitle()` unless a real level is running, which an
  invite never opens over — from `M_Join_Remote_Close` and from `M_Join_Remote_Connect`'s
  nobody-joined return.
- **Test**: `nojoinmaster` — Mark's direction (the member hosts, the master is invited), record demos
  kept, and the host started `NJDELAY` (24) seconds late so the invite lands during the master's first
  demo, which the case checks from the status line before the invite. It then requires the master's
  last status after "invite is over" to be the attract cycle (`gamestate` 1 or 4, `menu=0`, `none`).
  Without the fix: `gamestate=0 ... menu=0 ... none`, red. With it: the title page, then the next demo.
  `nojoin noshow convert memberpress demojoin linkgame` pass, `make smoke` 5/5.

**The host "stutters a little bit when the Pi joins", right as play starts** (Mark, 2026-09-15). Not
the joiner's picture this time — the laptop's own game stopped for about half a second, roughly a
second into every linked game.
- **Cause: the joiner's screen wipe.** The wipe is a blocking loop in `D_Display` (up to 2 s) that runs
  no tics and services no network. The host can make at most `BACKUPTICS` (32) tics past the lowest
  `nettics[]`, and a client's acknowledgement (`resendfrom` = `cl_need_tic`) cannot run ahead of its
  own `gametic` by more than that either. The laptop loads MAP01 in 0.15 s and crossfades in 0.46 s; the
  Pi 3 loads in 0.25 s and melts for 1.7 s. So the laptop played 31 tics after its crossfade (tic 104
  = the Pi's 73 + 31), then stood still until the Pi's melt ended, and the Pi ran the 32 waiting tics
  in one pass. Identical cabinets wipe for the same time and never see it; nothing on the network was
  slow.
- **Found** with temporary per-pass logging (wall time, tics run, `maketic`, and each node's `nettics`)
  on the real pair: laptop hosting on offscreen OpenGL with a copy of its live home, the Pi joining
  under the dummy driver with a copy of its own (its live game was running; software 8bit will not start
  under `offscreen` there).
- **Fix**: the wipe loop calls `TryRunTics` on a joining cabinet (`netgame && !server`) — see
  `screen-wipe.md`. Hosts and local games are unchanged.
- **Measured** on the real pair, freeze on the laptop after its own crossfade: build before 572, 543,
  543 ms at one player a side and 286 ms at four; fixed build none in four runs (one and four a side),
  the laptop's only gap its own 460 ms crossfade. The Pi's first-5 s `-tictiming` error went from 1.02
  to 0.22 tic RMS, since it no longer runs a 32 tic burst.
- `-tictiming` gained **`stall_ms`**, the longest wait for a new tic in the window — the RMS averaged
  this freeze away (0.15 against 0.14).
- **Test**: `wipejoin` — the host with `screenlink "None"`, the joiner with `"Melt"` (which runs to its
  2 s limit under the dummy driver); the host's first `stall_ms` must be under 400. Build without the
  fix: 1118 ms, red.
- Mark played it: fixed.

**Needs a person** — not reached headlessly:
- An invite arriving while someone is in the other cabinet's **menus**, and while they are part way
  through the **guided control setup** (it should be abandoned exactly as Escape abandons it).
- Arcade death **in a linked game**: `G_Arcade_Death_Check` was written for one machine. (The idle
  timeout has since been covered headlessly: `idleshared`, `idleall`, `idlehost`.)
- A Deathmatch against a Campaign opened at the same time (should be two separate games).
- Pulling a cable mid-game, and playing on the Pi 3 over Wi-Fi for longer than a test run.

### Select Game Sync — what was built (2026-09-14)

Mark: "when someone changes the game (e.g. doomu -> doom2), the game should change on all cabinets if
they are not already playing a game, otherwise linked games of non-default games would be a
challenge" — then: "this needs to be an option ... Should only show up on the master. That way the
operator can decide", on a page of its own because more link settings are coming, and "there might
be an issue if the wads are not installed on the other system".

Invites only go to cabinets running the same game id (IWAD plus level pack), so before this a linked
TNT deathmatch needed someone to pick TNT on every cabinet first.

**What it does.** With **`link_gamesync`** on (the master's setting; off by default), a game picked
on the Select Game page of any cabinet — an IWAD, loading a pack, switching packs or unloading one —
is followed by every other cabinet that is on its attract screen or in its menus, the states an
invite may interrupt. A cabinet in a game, signing the board, on a join screen or in an operator
session follows once it is back to one of those. A cabinet that cannot follow stays where it is.

**Files**
- `svn1749/src/d_linksel.c` / `d_linksel.h` — the protocol and its state, on the game thread,
  `LKSEL_Ticker` from `LK_Ticker` after the score sync. Its three message types come in through
  `LKG_Ticker`'s event poll (one queue), which hands them over.
- `m_menu.c` — `cv_link_gamesync`; `M_Restart_Program_Ex` (a pack path to load after the restart, and
  `-linkselected`); `M_Link_Game_Id_Valid` / `M_Link_Game_Why_Not` / `M_Link_Follow_Game`; the
  **Cabinet Link Options** page (`LinkOptionsDef`, a generic menu with a wrapped explanation) and
  `M_Draw_ArcadeOptions`, which hides its Arcade Options row unless this cabinet is a master.
- `d_link.c` — `LK_PROTO_VERSION` **4** (a v3 cabinet would close the connection on the unknown
  message types; now it is refused at `AUTH` instead), and the page's red lines.

**The protocol** — three `ROUTE`d game messages:
- `GAME_SELECTED` (game id), a member to its master: a player picked this here.
- `GAME_SWITCH` (serial, game id), the master to a member. A member acts only on one whose source is
  its own master (the master rewrites a relayed member's source, so no member can order another).
- `GAME_CANNOT` (serial, game id, reason), a member to its master.

**Design decisions**
- **A pick is an event, not a standing target.** The master keeps the latest pick and a serial, and
  each cabinet deals with a pick once: it follows, or says it cannot. Treating the pick as "the group
  game" instead fought the level pack rules: a cabinet's idle timeout unloads its pack by restarting
  (`menus.md`), and a standing target would load it straight back, on a cabinet whose attract demos a
  pack makes wrong. A busy cabinet has not dealt with the pick yet, so it still follows when free.
  - **Superseded in part the same day**: that unload is now itself passed on as a pick (below).
- **The pick lives in the master's memory, and survives only the restarts it causes.** A pick that
  restarts the program adds `-linkselected`; the new process announces it (a member once the link is
  up, a master to itself). `M_Restart_Program_Ex` strips `-linkselected` from every other restart — a
  devmode toggle carrying it would re-announce an old pick to every cabinet. A master that restarts
  to follow a member's pick passes it too, so it still tells the members that had not followed yet.
- **Wads that are not installed.** Everything a game id names comes off the network, so it is only
  compared with what the cabinet has: the IWAD part must be one of the Select Game page's own `-game`
  names and `D_Game_Available`; the pack part is matched, case-insensitively and cut short the way
  `levelpack_name` is, against the `.wad` files in `legacyhome/levels/`, and must hold maps for that
  IWAD (`M_LevelPack_MapStyle`). No path is ever built from the id. The check runs **before** the
  restart — without it a cabinet re-execs with `-game tnt` and no TNT, which the selfcheck below shows.
- **Why it could not is shown where the operator looks.** The member replies `GAME_CANNOT`; the master
  draws `GAME SYNC: TNT NOT INSTALLED` / `GAME SYNC: NO LEVEL PACK DWANGO5` in red under that cabinet
  on the Cabinet Link page, and a master that cannot follow a member's pick draws its own under the
  list. **The first wording, `GAME SYNC: NO TNT HERE - FINAL DOOM: TNT IS NOT INSTALLED`, was cut at
  "TNT IS" on the page** — found on the OpenGL capture, the same way "REFUSED: LOCKED OU" was — so
  reasons are short, important words first, and use the `-game` name. A member re-sends its last
  `GAME_CANNOT` whenever its master comes back online: an operator restarting the master into
  `-devmode` to look at the page has restarted away the master's memory of it.
- **A pack into an empty slot loads in place**, as the page does it (`M_LevelPack_Add`, shared with
  `M_SelectGame`); anything else restarts with the pack as `-file`, by exactly the path
  `M_Scan_LevelPacks` builds, so the new process shows it as loaded.
- **An operator session passes a pick on but is never switched.** It is not a player (the presence
  table), and an operator mid-change must not be restarted from another cabinet.
- **Off means nothing is recorded**: a pick made while it is off is logged and forgotten, so turning it
  on later does not replay an old one.
- **The page is on a master only.** Arcade Options' last row, hidden by its drawer from the live role
  (the role can change underneath it on the Cabinet Link page), the way `M_Update_EndGame_Row` hides
  End Game. Last, because a hidden row still takes its place.

**Verified** — `tools/linktest.sh`, five new cases, all pass (`-linktest -linkselectat S name` picks
an IWAD or pack on the Select Game page S seconds in; `LINKSEL` / `LINKSELPEER` status lines, `wads=`
the number of wads really loaded, since `addfile` reports only to the console):
- `gamesync` — a member picks TNT: the master follows (and still holds the pick after its restart),
  the other member follows, and each engine starts exactly twice.
- `gamesyncoff` — the same with the setting off: logged, nothing follows.
- `gamesyncmissing` — the master picks TNT; one member has no `TNT.WAD` (and a `HOME` with no
  `~/games/doom`, which the IWAD search also reads): it says why, does not restart and does not claim
  to be switching; the master shows `GAME SYNC: TNT NOT INSTALLED`; the other member follows.
- `gamesyncbusy` — a member in a level: the master logs that it waits, the member switches only after
  its game ends (`-linkcmdat 40 exitgame`).
- `gamesyncpack` — the master loads a pack built from DOOM2.WAD's own MAP01; a member holding it as
  `SyncPack.wad` loads it in place (one more wad, no restart); one without it says `NO LEVEL PACK`.
- **Each shown red** with its protection taken out: the setting ignored (`gamesyncoff`), the installed
  check removed (`gamesyncmissing`: the member restarted into a game it lacks), busy cabinets switched
  (`gamesyncbusy`: switched mid-game), packs always restarted (`gamesyncpack`), and the master
  following without `-linkselected` (`gamesync`: fails on "still holds the pick" alone).
- `pair unbound passcode fakemaster garbage bigframe linkgame nojoin menusetup scores idlehost` pass;
  `make smoke` 5/5; the pages captured in OpenGL (Arcade Options on a master and a member, the options
  page, the red line on the Cabinet Link page).
- **Found on the way: `unbound` had been testing nothing since Phase 2.** `tools/linktest_peer.py`
  kept its own copy of the protocol version, still 2 after the engine went to 3, so the master refused
  its proof on the version byte before checking whether it was bound to the session — the exact trap
  Phase 3 recorded, again. With the byte back at 2, `--selfcheck unbound` **stays green**. The peer now
  reads `LK_PROTO_VERSION` out of `d_link.c`, and the selfcheck goes red.
- `tools/menufit-test.py` skipped any page whose drawer is not literally `M_DrawGenericMenu`, so giving
  Arcade Options its own drawer silently dropped it from the check. It now also measures pages whose
  drawer calls it (31 → 48 pages), and skips, with a line saying so, the one that places rows by symbol.

**Needs a person**: pick a game on the Pi's Select Game page with the laptop on attract (and the other
way round); pick one while the other cabinet is mid-game and watch it switch after; a cabinet without
Plutonia picking Plutonia elsewhere.

#### Faster switching (2026-09-14)

Mark, after playing it: "the amount of time it takes to restart the other system is a little
disappointing ... the multiplayer join screen pops up almost immediately, but this takes like 5-10
seconds". A timeline of three engines on copies of the laptop's live home (`ts`-stamped output, the
member picking TNT) showed where the laptop's part went:

| step | before | after |
| --- | --- | --- |
| pick → the other cabinet starts its switch | **3.8 s** | 0.0 s |
| restart → link running again (startup) | **2.0 s** | 0.27 s |
| of which: md5 of the two soundtrack wads (418 MB) | ~1.8 s | cached |
| TLS + passcode proof after reconnecting | 0.9 s | 0.9 s (nobody waits on it now) |
| the `SWITCHING GAME...` hold and shutdown | 1.0 s | 1.0 s (kept: it says what the black screen is) |

- **The other cabinets were waiting for the picking cabinet's whole restart.** A pick that restarts
  only reached the master from the *new* process, once it had started, connected and proved the
  passcode. `M_Restart_Program_Ex` now calls **`LKSEL_Before_Restart`** with the game id the new
  process will have (a member sends `GAME_SELECTED`, a master sends its `GAME_SWITCH`es), just before
  the splash, whose 700 ms hold gives the link thread time to send it; the link thread also drains its
  outbox and flushes every connection when told to stop. `-linkselected` is still passed, as a
  fallback, and a repeat of the pick the master already holds changes nothing.
- **Every start hashed every wad in full** (`W_Load_WadFile`; the netcode and the link compare the
  md5s). `md5sum` alone takes 1.27 s over those four files on the laptop. The md5 is now remembered in
  **`legacyhome/wadmd5.txt`**, keyed by path, size, modification time and inode, and written with
  `M_Atomic_Write_*`; a replaced or edited wad changes one of those and is read again. This speeds up
  every start, boot included, not only a switch. On a Pi 3 hashing an 18 MB IWAD is slower still.
- `gamesync` checks the pick went out before the restart. The Pi's own figures were not measured.

#### Returning to attract drops the pack on every cabinet (2026-09-14)

Mark: "when I load a wad say dwango5 and then start a game and then 'end game' from the menu, it
unloads the wad and restarts on that cabinet but not the other one". By the design above that was
intended — only a Select Game pick was passed on — and it was wrong: the two cabinets ended on
different game ids, so neither could invite the other any more. The two restarts that unload a pack on
the way back to attract, `M_EndGameResponse` and the idle timeout in `G_Ticker`, now pass
`link_selected`, so the bare IWAD goes out as a pick. A cabinet still in a game follows when free; a
second cabinet unloading its own pack sends the same pick, which the master ignores as a repeat.
- Case `gamesyncunload`: both cabinets on a pack, the master starts a level and idles out
  (`idletimeout 15`); both end on `doom2`. **Fails on the build before** with the field symptom — the
  follower never drops the pack. End Game is the same call and is not driven headlessly.

#### A cabinet on attract takes the master's choice (2026-09-15)

Mark: "it seems there is still a possibility of having two cabinets each running a different
game/wad, e.g. if a cabinet plays a single level and then the other cabinet changes the game ... if it
goes into attract mode, it should change to the master's choice".

**The standing rule, as a backstop to the events.** `lksel_send_defaults` (master, every tick): any
online member whose presence is **`IDLE`** — attract, no menu open — and whose game is not the
**master's choice** is sent `GAME_SWITCH` with **serial 0**. The choice (`lksel_group_game`) is the
latest pick while one stands, else the master's own game, so a pick made on a member while the master
was busy is not undone before the master follows it. A member takes a serial-0 switch only when `IDLE`
(someone in the menus has not left the cabinet), and handles it as any switch — including a missing
wad (`CANNOT`, and a copy when Copy Missing Wads is on). No new message type, so no protocol change.
- 5 s grace after a member connects (its own pick may still be on the way, the -linkselected fallback);
  resent after 20 s if not taken; **5 minutes** after it said it cannot run that game.
- A member with a pick still being delivered (`send_switches` has not finished with it) is left to that.
- **With Select Game Sync on, the master's game is every member's boot game** — a member booting into
  its own Boot Game switches after the grace. That is what "left alone, default to the master" means.

**What it exposed: a dropped pack overrode a newer pick.** Mark's own example turned out to have a real
cause, not just a missing backstop. Yesterday's fix sent the bare IWAD as a *pick* when a cabinet
unloaded its pack on the way back to attract. So a member playing on a pack while TNT was picked on the
master would end its game, announce `doom2`, and pull the master and everyone else back off TNT.
- A pack drop is now **`LKSEL_Unloading`**, called by `M_Restart_Unload_Pack` (End Game and the idle
  timeout): a member sends `GAME_SELECTED` **one byte longer** (flag 1: only dropping its pack), and the
  master applies it only when the choice *is* that IWAD with a pack (`lksel_drops_pack`). Otherwise it
  logs "the link stays on tnt" and the member takes TNT on attract. An older master ignores the longer
  message. A master dropping its own pack does the same test, and when the choice has moved on it
  **restarts straight into the choice** instead of into the bare IWAD.

**Verified** — `tools/linktest.sh`:
- `gamesyncattract` — a member boots into TNT, the master runs Doom 2, nobody picks: the member switches
  to Doom 2 ("the master's game"). **Red** with the defaults switched off.
- `gamesyncunloadkeep` — Mark's case: the member plays on a pack (`-warp`), TNT is picked on the master,
  the member idles out and drops its pack: the master logs that the link stays on TNT, both end on TNT.
  **Red** with a pack drop treated as a pick again — the master was switched back to Doom 2.
  - The first version started the member's level from `-linkcmdat` six seconds in, and the new rule
    switched the member to the master's game in those six seconds on attract — the feature working,
    the test not testing what it said. `-warp` puts it in the level from boot.
- All eleven Select Game Sync cases and `linkgame` pass; `make smoke` 5/5.

#### Copy Missing Wads (2026-09-14)

Mark: "Can there be an option to copy wads from the master to the members? Not sure the best landing
place for them? Maybe alongside the binary would be the best location (and less OS dependent)?"

**This reverses a line of the original plan** ("A file written to disk because a network peer said so
has no place on a cabinet"), on the operator's say: it is **`link_copywads`**, the master's setting,
Off by default. What still holds from that rule is that the *game netcode's* downloads stay off in a
linked game; this is a separate, narrow path.

**What is copied, and where.** Only what a pick needs and the member lacks (`M_Link_Missing`): the
IWAD, then the level pack if that is missing too. The master finds its own copy by the Select Game
rules (`M_Link_Wad_Path`: `D_Game_Path` for an IWAD, `M_LevelPack_Find` for a pack) — a path never
crosses the wire. The member takes only a name that part may have (`M_Link_Wad_Dest`: an IWAD name from
`game_desc_table` for that game, or `<pack>.wad`; letters, digits, `. - _ +`) into one of two places:
- **IWADs: `wads/` beside the program** (`D_Progdir_Wads`). The engine already searched it
  (`owner_wad_search_order`, `doomwaddir[2]`, ahead of `~/games/doom`), on every OS, so no search
  change was needed — which is what makes "alongside the binary" the right landing place, as Mark
  guessed; the subfolder keeps the program's own directory tidy.
- **Packs: `legacyhome/levels/`**, the only place Select Game lists them.
Nothing is written over an existing file.

**The transfer** (`d_linksel.c`, "Copy Missing Wads") rides the score sync's channel — member↔master
only, pulled by the receiver — with its own message kinds from 32 up, which `LKS_Ticker` hands over.
`WANT` (what, game) → `OFFER` (what, md5, size, name) or `NONE` (reason) → `GET` (md5, offset, 12
chunks) → `DATA`. The member writes `<file>.part`, runs the md5 as it goes, fsyncs, compares, then
renames. **The md5 is the one `W_Md5_File` remembers**, so offering a wad the master has loaded costs it
no read; the link is TLS between cabinets that proved the passcode, so the check only has to catch
damage, not an attacker. 192 MB cap.
- The member re-sends `WANT` every 3 s until answered (30 s), and on reconnecting: **the first version
  lost every request**, because the master that picks a game restarts to follow it, and the `WANT`
  went into a closing connection. A copy whose master goes away pauses, and resumes from where it got
  to when the master offers the same md5 again.
- **A copy pauses while its cabinet is in a game** (no GETs until it is idle or in menus): a copy must
  never cost a player a frame.
- Once copied, the member follows the pick if it is still the latest and the cabinet is free.

**Verified** — `tools/linktest.sh`:
- `gamesynccopy` — the master picks TNT; a member without it (HOME with no `~/games/doom`) copies
  `TNT.WAD` into `wads/`, restarts with it *loaded from there*, identical to the master's file, no
  `.part` left; the master's page reads `COPIED TNT.WAD`. 18 MB in about 20 s on the laptop.
- `gamesynccopypack` — the same for a pack into `legacyhome/levels/`, then `doom2+syncpack`.
- `gamesynccopybad` — `-linktest -linkcorruptwad` damages one byte on the master: refused, nothing left
  in `wads/`, still on Doom 2. **Shown red** with the md5 comparison removed: the damaged file was
  installed, and the engine then crashed loading it.
- `gamesyncmissing` now also checks that with copying off the member says so.
- **A test switch that corrupted every copy**: `lkc_test_corrupt` was first a `boolean` set to `-1` for
  "not read yet". `boolean` is an enum here and can be unsigned, so `< 0` was never true and the flag
  stayed on: every copy "arrived damaged" and was correctly refused. Use `int` for a tri-state.
- All eight Select Game Sync cases, `pair unbound linkgame iwadname iwadversion musicwad scores
  scorebusy` pass; `scores` failed once in a batch of fifteen beside an 18 MB copy and passed alone and
  again beside it — not explained. `make smoke` 5/5; the options page captured in OpenGL.

**Not done**: throughput is the sync channel's (a window of 12 × 4 KB, paced by the game loop); the soundtrack
wads a *netgame* may want are not copied (they are not required, see the music wad fix above).

**Not done**: the picked game is not persisted — a master switched off forgets it, and a cabinet that
boots later keeps its Boot Game. Two picks made on two cabinets within the same second may each
restart the other once before settling on the one the master heard last.

#### The whole list, in the background (2026-09-24)

Mark: "I'd like it to sync wads automatically in the background instead of just on demand. I want
my whole list synched up." On demand meant a member only found out it lacked a game when someone
picked it, and then the pick sat waiting 20 seconds or more per IWAD while it copied.

**Same setting, new meaning.** `link_copywads` now means "members keep the master's whole Select
Game list", not a new cvar beside it: the cabinet's config already had it `On`, and a second switch
would have left the two able to disagree. That changes what an existing `On` does, which the
new-cvar default rule normally forbids, but here the change *is* the request.

**What "the list" is.** `M_Link_Wad_List` (`m_menu.c`): each Select Game IWAD the master has, then
**every** `.wad` in its `levels/`, not only the packs the running game can load (Select Game's own
scan filters by map style; a Doom 2 master still holds Ultimate Doom packs). Each pack is named
under an IWAD the master has that can load it (ExMy → `doomu`, MAPxx → the first of
doom2/plutonia/tnt it has), because the transfer below speaks in game ids; a pack no IWAD there can
load is on nobody's list and is left out. Sorted, as the page shows them. One `LIST` message holds
102 ids, past the page's 4 IWADs + 64 packs.

**Two new sync kinds, no protocol bump.** `LIST_ASK` (37, member → master) and `LIST` (38: count,
then the ids; count 0 = copying is off). An older master ignores kind 37, so a new member simply
never gets a list and behaves as before. Each file then travels by the unchanged
`WANT`/`OFFER`/`GET`/`DATA` path, so the md5 check, the `.part` + rename, the destination rules and
"never over an existing file" all apply unchanged.

**When it runs** (`lkb_member_tick`, `d_linksel.c`):
- Only while **both** ends are quiet: `IDLE`, `MENU` or `DEVMODE` (`lkc_quiet`). The member's state
  was already checked; the **master's** is new, and it matters: the master reads each chunk off disk
  on its game thread, and building the list opens every pack's directory. A background copy under
  way pauses when either end stops being quiet. A pick's copy still waits only for its own
  cabinet, as before.
- One `M_Link_Missing` check per tic, because each one runs `Search_doomwaddir`.
- The list is asked for on connecting (so a restarted master is re-read), every five minutes (a
  pack dropped into the master's `levels/` arrives without anyone touching anything), and pushed
  by the master to every member when the setting is turned **on**, so an operator does not wait
  five minutes to see it work.
- A file that failed is not tried again until a list that differs, or the next five-minute list
  after a failure. `NOT ENOUGH DISK SPACE` and `COPY MISSING WADS IS OFF` stop the pass until then.

**A pick comes first, and takes over rather than restarting.** `lkc_start(game, follow)` replaced
`lkc_want`. A background copy of the same *file* (`lkc_part_key`: `tnt` and `tnt+x` both need
TNT.WAD) becomes the pick's copy, progress and all; any other background copy is dropped for the
pick. A background copy never displaces anything. `wadsyncpick` shows the takeover surviving the
master restarting to follow its own pick mid-copy: 9 MB in, the connection drops, and the copy
resumes from 9 MB.

**A copied file shows on Select Game without a restart.** The page was built once, in
`M_Configure`; that block is now `M_GameSelect_Build`, run again by `M_GameSelect_Refresh` once
the menus are closed (never under a player's cursor). The rescan has to keep which pack is loaded:
`M_Scan_LevelPacks` rebuilt `levelpack_isloaded[]` from the command line alone, which is right at
startup and wrong for a pack added in place, so it now carries the old flags over by path. And
`OPT_selectgame` is now un-hidden too, since a cabinet that booted with one game can have two.

**Also fixed on the way**, both of which only matter once copies run unattended for minutes:
- **Free space.** Nothing checked it. On the Pi's SD card, a full disk means the next config or
  score save fails. A copy is now refused unless it leaves 256 MB (`lkc_free_space`, `statvfs` on
  the destination directory; on Windows, `I_GetDiskFreeSpace`, which asks about the current drive).
- **A copy could stall for ever.** The master serves `LKC_OFFERS` files at once and reuses the
  oldest slot; a member whose offer was evicted sent `GET`s that were silently ignored, and it only
  gave up if the master went *offline*. Now ten seconds without data (not counting pauses) re-sends
  `WANT`, and the master's re-offer of the same md5 resumes the copy. `LKC_OFFERS` went 4 → 8.
- The master's page notes say **`WAD SYNC:`**, not `GAME SYNC:`: the master cannot tell a pick's
  copy from the background's, and most copies are now the background's. The member's own note uses
  `GAME SYNC:` for a pick's copy and `WAD SYNC:` otherwise.

**Verified** — `tools/linktest.sh`, four new cases (`wadsynccabs`: the master holds a MAPxx and an
ExMy pack, built by `mkpack`, which now takes an IWAD and a map; the follower lacks both and TNT):
- `wadsync` — nothing is picked: the member is sent 6 ids, copies TNT into `wads/` and both packs
  into `levels/`, all byte-identical, no `.part` left; Select Game went 3 → 4 → 5 rows with no
  restart; the member stays on Doom 2 and starts once.
- `wadsyncbusy` — the master in a level until 35 s: its first `copying` line comes after its
  `exitgame`.
- `wadsyncoff` — off: the member hears "not copying"; turned on from the master's console at 25 s,
  it gets the list at once and copies everything.
- `wadsyncpick` — the takeover above, TNT started exactly once, then the switch.
- **Each shown red** in one build with its protection taken out: the master-state gate
  (`wadsyncbusy`: copy line 52, game ended line 183), the push on turning it on (`wadsyncoff`), the
  takeover (`wadsyncpick`: TNT started twice).
- All eleven Select Game Sync cases (the copy cases now exercise the takeover too, since their
  follower lacks TNT from the start), `pair linkgame scores scorebusy iwadname` pass; `make smoke`
  5/5. `unbound` failed once in the batch of 21 run three at a time, and passed alone; it is a TLS
  handshake test that touches no wad code.

**Not done**: copies go master → members only; a pack only a member has stays there, and a member
never pushes. A pack changed on the master under the same name is not re-sent, since an existing
file is never overwritten. Throughput is still the sync channel's (about 1 MB/s on the laptop).

### Windows port — what was built (2026-09-15)

Until now a Windows build always compiled the link out: `tools/build.ps1` never probed OpenSSL, and
`d_link.c` was POSIX sockets only. **Read this before touching socket code in `d_link.c`**, which now
has to compile both ways. The Makefile already had the Windows libraries (`-lssl -lcrypto -lws2_32
-lcrypt32`).

**Built**
- **`tools/build.ps1`** probes OpenSSL by linking, like build.sh, and it is optional the same way: a miss
  prints the `pacman` line and builds with the link compiled out. `-InstallDeps` installs it, including
  when it is the only thing missing. The script writes `HAVE_LINK=1` into `make_options`, *appending*
  it to a reused file that has no `HAVE_LINK=` line. An explicit line is the operator's; `HAVE_LINK=1`
  with OpenSSL gone is warned about. Its closing message gives the firewall rule (below).
- **CI** fails the Windows job if `make_options` lacks `HAVE_LINK=1` or `libssl-*.dll`/`libcrypto-*.dll`
  were not staged. The link is optional to the script, so a release without it would otherwise build
  green. The DLL walk finds the two OpenSSL DLLs by itself: the first CI run staged sixteen.
- **`d_link.c`, the socket layer.** The rest of the file was already portable, and `d_linkgame.c`,
  `d_linkscore.c` and `d_linksel.c` already built on Windows (they do not test `HAVE_LINK`). The
  game's UDP channel goes through `i_tcp.c`, which always had Winsock.
  - The Winsock headers go after `doomincl.h`, as in `i_tcp.c`, since `doomtype.h` has already
    pulled in `windows.h`. An `#error` stops a build with `_WIN32_WINNT < 0x0600`, where `WSAPoll` and
    `inet_ntop` do not exist.
  - Descriptors stay `int`. A `SOCKET` handle value fits, `INVALID_SOCKET` truncates to -1, and
    OpenSSL's `SSL_set_fd` takes an `int` on Windows too.
  - `lk_closesocket`/`lk_poll`/`lk_sock_errno` map to `closesocket`/`WSAPoll`/`WSAGetLastError`.
    `ioctlsocket(FIONBIO)` replaces the `fcntl` switch.
  - **A non-blocking connect in progress is `WSAEWOULDBLOCK`**, not `WSAEINPROGRESS`, which means
    something else on Winsock. Mapping it to the obvious name would fail every member's connect.
  - `lk_sock_strerror`: `strerror()` knows none of Winsock's codes and would print "Unknown error" for
    all of them, so on Windows the text comes from `FormatMessage`. The error is also captured
    before the failed socket is closed, which could overwrite it. That was a latent bug on Linux
    too.
  - **The wake pipe is a loopback UDP socket pair on Windows** (`lk_wake_open`), because `WSAPoll`
    accepts sockets and nothing else. A packet from anything else on 127.0.0.1 only wakes the
    thread early.
  - `WSAStartup(2,2)` at the top of `LK_Init`, before `gethostname`. `i_tcp.c` starts Winsock only
    for a network game, and asks for 1.1. The calls are counted, so the two do not interfere.
  - The listening socket uses `SO_EXCLUSIVEADDRUSE`, not `SO_REUSEADDR`. On Windows `SO_REUSEADDR`
    lets another program bind the port while the master is listening on it, and Windows does not
    hold a listening port in TIME_WAIT, which is the only reason POSIX needs it.
  - No `SIGPIPE` (a failed send just fails) and no `fchmod(0600)`: Windows has no mode bits, so
    `link.cfg` and the key take the permissions of the folder `legacyhome` is in.
- **Program restart on Windows** (`M_Restart_Windows`, `m_menu.c`). Select Game Sync, game switching,
  pack unloading and Devmode Restart all restart the program, and `execvp` is unusable here. The C
  runtime's exec starts a new process and ends this one, but it joins the arguments with plain
  spaces, so a `-file` pack under `C:\Users\...\My Games\` came back as two nonsense file names and
  the restart silently lost the pack. It also passed this process's handles, sockets included, to
  the new one. Now:
  - `M_Restart_Quote_Arg` quotes each argument by the `CommandLineToArgvW` rules.
  - The arguments are UTF-8 from SDL's WinMain and are converted for `CreateProcessW`.
  - The executable comes from `GetModuleFileNameW`, not `argv[0]`.
  - Handles are not inherited, and the process then `exit(0)`s as `I_Quit` does.
  - **A new process does not get the foreground for free**, which is what made Devmode Restart come
    back behind whatever else was open — intermittently, since it depended on what held the
    foreground at the moment this one let go of it. `AllowSetForegroundWindow` here and
    `I_Raise_Window` in the new process fix it; written up in `menus.md`.
  - **Consequence: the restarted program is a new process.** Anything that launches the cabinet
    and waits for it to exit sees an exit at every restart. A "restart it when it quits" wrapper
    would start a second copy. Launch the exe directly, from a shortcut or the Startup folder.

**Verified**
- Linux, the same tree with `HAVE_LINK=1`: builds with no new warnings. `tools/linktest.sh -j 3`
  passed 57 of 61 cases; `scoreslarge` skips without `SCOREHOME`. It ran alongside a compile, and
  `memberpress`, `musicwad` and `demojoin` failed, all three at the point where a game is joined.
  Re-run alone (`-j 1`), all three passed. This covers the POSIX side of every shim, whose behaviour
  must not have moved.
- `tools/restartquote-test.py --selfcheck` extracts `M_Restart_Quote_Arg` verbatim, compiles it
  natively and round-trips 18 awkward arguments plus 3000 random argument lists. The awkward ones
  include spaces, embedded quotes, trailing backslashes, the empty argument and non-Latin names.
  The splitting side is a Python implementation of the Windows rules, itself checked against the
  five examples Microsoft publishes. The self-check reinstates six quoting bugs and each goes red.
- Windows (PR #17's CI run): the probe found OpenSSL and wrote `HAVE_LINK=1`. `d_link.c` compiled
  with `-DHAVE_LINK` under MinGW-w64 ucrt64 with no warnings, the exe linked, and sixteen DLLs were
  staged, `libssl-3-x64.dll` and `libcrypto-3-x64.dll` among them. That is **compile and link only**.

**Not verified — needs the Windows box (the pinned step 4)**
- That any of it runs: that a Windows master accepts the Pi and a Windows member reaches the Pi,
  presence, invites, a linked game, the score sync, Select Game Sync's restart and Copy Missing Wads.
- `PEM_write_PrivateKey`/`PEM_read_X509` are handed a `FILE *` opened by the game. That is safe only
  because MSYS2's OpenSSL and the game share the UCRT. An MSVC-built OpenSSL would need
  `OPENSSL_Applink`. The first start creates `link/cabinet.key`, which is the test.
- **Windows Firewall** prompts the first time a master listens. On a fullscreen cabinet the prompt is
  behind the game, and the link just never connects. Allow it once from an administrator prompt:
  `netsh advfirewall firewall add rule name="Doom Legacy Arcade" dir=in action=allow program="<run dir>\doomlegacyarcade.exe"`.
- Before Windows 10 version 2004, `WSAPoll` never reports a refused non-blocking connect. A member
  pointed at a master that is not running then says "timed out before authenticating" after 10 s
  instead of "connection refused".

**Not done**
- A demo the attract cycle is playing still cannot be replaced by the score sync on Windows (see
  Phase 2).
- The determinism check from Phase 0 step 4.

#### First run on a real Windows cabinet (2026-09-15)

Mark set up a Windows member (DESKTOP-202K7KS) against the laptop master. It authenticated, then went
quiet and was dropped as "stopped responding", over and over; a linked game "timed out"; and the
Cabinet Link page said **SCORES: DIFFERENT WADS**. These turned out to be two separate problems.

**1. A stamp later than `now` read as 49 days old** (fixed, PR #18). `lkt_main` reads the clock
once per pass, then `lkt_member_connect` stamps `c->started` with a fresh reading. The handshake
check `now - c->started` is unsigned, so when resolving and connecting crossed a millisecond it
wrapped, and the new connection was closed as "timed out before authenticating" straight after
`connect()`. On Linux a numeric address resolves in far less than a millisecond, so it never
showed; on Windows it hit most attempts. `LKS_Ticker` had the same shape: messages stamp `last_ms`
after `now` is read, so a transfer that had just answered read as stalled. Every elapsed-time check
in `d_link.c` and `d_linkscore.c` now goes through `lk_since`/`lks_since`. `d_linkgame.c` already
had `lkg_since` for exactly this, found on the Pi. **Any new timer in the link code needs the same
helper**; the stamp and the read are never in a guaranteed order.
- **Reproduced three ways.**
  - The Windows CI build under Wine, as a member of a Linux master: "timed out before
    authenticating" on every attempt, and the master logged "TLS handshake failed".
    `WINEDEBUG=+winsock` showed `closesocket` straight after the in-progress `connect`, with no
    `WSAPoll` between. That is what pointed away from Winsock and at the timer.
  - On Linux with a temporary `SDL_Delay(3)` in `lkt_member_connect`: `linktest.sh pair` fails
    with the old subtraction and passes with the fix.
  - The PR #18 Windows build under Wine: it pairs on the first attempt, stays online, is invited
    to Deathmatch and plays a two-player linked game (about 2,100 sealed packets each way, 0
    dropped).
  - `linktest.sh -j 1 pair passcode lockout linkgame memberpress scores scoreclear gamesync` passes
    on Linux; those cases exercise every timer the fix touched. The full suite was killed twice for
    low memory on the laptop, and has not been run on this change.
- **Wine is a good first test for the Windows build**, though not a substitute for Windows: its
  Winsock is a layer over Linux sockets. The setup that works: a private `WINEPREFIX`,
  `DISPLAY=`, `SDL_VIDEODRIVER=dummy`, `drawmode "Software 8bit"`, the package's `legacyhome`
  with a `link.cfg`, and the IWADs linked beside the exe. Use ports away from 5230 upward, which
  `linktest.sh` uses.
- Not proven: that the clock bug is the *whole* of "stopped responding", which was the master's
  view of an authenticated member going silent. The Wine member never went silent after the
  fix. If a real Windows cabinet still does, its own log is the next thing to read. Run the exe
  from a command prompt with `> out.txt 2>&1`, since the exe has no console.

**2. SCORES: DIFFERENT WADS is a different IWAD, and it also blocks the linked game.** The score
fingerprint is the MD5 of every loaded wad (`D_Net_Wad_Md5s`). The laptop's `legacy.wad` and
`dogs.wad` match the repository's, which is what the Windows package ships, so the difference is the
IWAD. The laptop has Ultimate Doom 1.9 (`DOOM.WAD` `c4fe9fd920207691a9f493668e0a2083`) and Doom 2 v1.9
(`25e1459ca71d321525f84628f45ca8cd`); a copy from the 2024 Steam/GOG re-release, for one, has
different contents under the same name. The same mismatch refuses the join. The joiner gets the
on-screen "You cannot connect to this server since it uses a different version of DOOM.WAD", and
the host waits out `jointime` and plays alone, which from the host looks like a timeout.
- Verified under Wine with the member on Doom 2 v1.666. The link stays online throughout, so a
  wad mismatch does **not** cause "stopped responding". The member reads as `menu` (the message
  box), and the host starts with one player.
- Copy Missing Wads does not help: it copies an IWAD a member *lacks*, never one it has in
  another version.
- **In the event it was not the IWAD.** Mark copied the laptop's `DOOM.WAD` and `DOOM2.WAD` beside
  the Windows exe, confirmed the MD5s, and the page still said DIFFERENT WADS. It was
  `legacy.wad`: the laptop was running an old copy.

**3. Which wad differs is now on the page** (2026-09-15, the `link-wad-diff` branch). Each score
manifest carries a `wad <md5> <name>` line for every fingerprinted wad (`lks_my_wads`, from
`D_Net_Wad_Md5s`, which now also hands back the paths). A cabinet from before these lines skips
them, and one that sends none still gets plain DIFFERENT WADS.
- `lks_explain_wads` matches by content first, as the netgame's IWAD check does, then by name. It
  gives `SCORES: DIFFERENT DOOM2.WAD` (same name, other bytes), `SCORES: legacy.wad ONLY HERE`,
  `SCORES: ONLY THERE: pack.wad` or `SCORES: WADS IN ANOTHER ORDER`.
- Both lists are printed once per change as `LINKLOG Scores: <cabinet>: <md5> <name>` lines, not
  with every manifest.
- `linktest.sh scorewads` covers a member on Doom 2 v1.666: both sides name DOOM2.WAD, both MD5s
  are printed, and the difference is printed once. `scorewadextra` covers a member with an extra
  one-lump PWAD, and checks the message on each side.
- Still not done: the netgame's own refusal on the joining cabinet names only the IWAD.

**4. SCORES: DIFFERENT SETTINGS between two identical cabinets.** Once `legacy.wad` matched, the
page said this instead, and the two configs were identical. `lks_rules_hash` read the *effective*
values (`.EV`) of rocket trails, view height and the invulnerability sky. An attract demo sets
those from its own header while it plays, and `playdemo_restore_settings` puts them back only when
it ends. A stock Ultimate Doom demo runs the sky as Vanilla (instrumented: `sky=1/0`, value/EV, mid
demo). The manifest is cached for minutes, and the other cabinet's is judged whenever it arrives,
so a cabinet part way through a demo reported a different ruleset. The hash now reads `.value`,
the configured setting. Outside a demo that equals `.EV`, so a cabinet on the older build still
agrees. `tools/hsmerge-test.py` accepts either form.
- **Unseen until now because the link tests run Doom 2**, whose own demos are v1.06 and refused, so
  no case ever had an attract demo playing. `linktest.sh scoreattract` runs two identical
  Ultimate Doom cabinets out of step: the member starts 20 s after the master, whose first demo
  begins about 16 s in. It failed before the fix and passes after. Run in step, both play the same
  demo at once, the two wrong hashes agree, and it passed against the bug, which is why the
  stagger is there.
- Writing that case showed a second bug. The score sync copied a cabinet's name once, when the
  cabinet was first listed, which can be before its name has arrived. It then stayed blank for
  good (`LINKSCORE peer=`). `LKS_Ticker` now refreshes the name from the link's list every tick.

**5. The Windows cabinet was online but not invited, and did not follow game changes.** It was
in a `-devmode` session. `lk_compute_state` reports DEVMODE, and by design an operator session is
neither invited (`LKG_Would_Invite`, `lkg_on_invite`) nor switched (`lksel_can_switch_now`).
Restarting it out of devmode fixed both. Its `link_gamesync "Off"` was not the cause: on a member
the setting is never read; only the master's counts.

**6. "Both locked in, but the countdown still ran to 0"** (reported 2026-09-15; not reproduced).
Lock-in starts a linked game early only when every panel that pressed in, on every cabinet, has
locked in (`M_Join_Check_All_Locked`, with `LKG_Remotes_All_Locked` for the others). Each remote
re-sends its joined and locked counts every second, and the host re-checks on each.
- Every arrangement tried starts at once:
  - `linkgame`: the joiner locked in first.
  - New `lockinlate`: the host locked in first, the joiner 5 s later over the link
    (`-linklockafter S`).
  - New `lockinthird`: the same with an idle third cabinet also invited.
  - The Windows build under Wine, as the late joiner and as the host.
  
  In each, the host starts about 3 status lines (about 6 s) after inviting, against a 30 s
  countdown.
- What does hold it, by design and invisibly: a panel that pressed in and never locked in, on
  either cabinet. A second pad, or a key bound to another panel's fire, is enough.
- **Built: the host now logs it.** Each change in a remote's counts is logged
  (`LINKLOG Cabinet Link: join screen: DESKTOP has 1 in, not all locked in`). When the countdown
  runs out with someone locked in, the host logs each of its own panels still choosing
  (`LINKLOG Join screen: the countdown ran out waiting on panel 2 here`) and each cabinet not
  locked in (`LKG_Log_Waiting`). `memberpress` checks the remote lines. The next time this
  happens, the host's log says what it was waiting for.

**7. Linked games with the Windows cabinet time out: it keeps going silent** (2026-09-15; open).
The laptop master's log shows the Windows member authenticating and then, 15 s later, "stopped
responding". It repeats all session, on the attract screen as much as in a game, while the Pi on
the same master never drops. A drop mid-game ends the linked game. Invites and the join screen
still work, because they happen in the seconds after each reconnect.
- **Not yet traced.** The master's "stopped responding" means *it* received nothing for 15 s with
  the connection still open. Had the member closed it, the master would say "disconnected" or
  "connection lost". Under Wine, the Windows build never went silent. The laptop's Dropbox LAN
  sync with the same Windows machine is healthy (MSS 1460, near-zero retransmits), so a plain
  network fault is less likely.
- **Built to trace it** (the `link-silence-trace` branch):
  - **`-logfile <file>`** (`CON_Logfile_Write`, `console.c`): everything that goes to the
    terminal, appended with a wall-clock time per line and flushed per write. The Windows exe
    has no console, so this is the only way to see its side. It survives a program restart
    because the argument is kept.
  - **`lkt_log_trouble`**: when a connection is dropped for silence, or a receive or send fails
    on an online one, it logs:
    - ms since last heard and last sent, with the TLS and socket error codes;
    - bytes and messages each way, and the last message type each way;
    - bytes still waiting to send, send stalls (`WANT_WRITE`), bytes unread in this end's socket
      (`FIONREAD`) and in TLS;
    - on Linux, `TCP_INFO`: unacked, retransmits, rtt, and the kernel's own last data in/out.
  - Reading it: bytes unread in the socket means this end stopped reading; retransmits climbing
    means packets are lost; neither, with the kernel quiet too, means the other program stopped
    sending.
  - `linktest.sh silentmember` freezes a member with `SIGSTOP` (the engine, not its `timeout`
    wrapper, which the first version froze by mistake) and checks all four lines on the master,
    and the member's timestamped `-logfile`. On it the master reads "heard 15072 ms ago, sent
    5025 ms ago", "0 bytes unread in socket", "tcp unacked 0, retransmitting 0". That is the
    signature of the other program going quiet with its kernel still acknowledging.
- Note that OpenSSL 3 reports a peer that vanished without a TLS close as `SSL_ERROR_SSL`
  ("ssl err 1"), not `SSL_ERROR_SYSCALL`, so a killed cabinet reads as "receive failed ... ssl
  err 1".
- In the next session, run with `-logfile` on both, the link never dropped. Still open; the
  diagnostics stay in for when it does.

**8. Twelve players: one cabinet's four had no name, colour, weapon switch or autoaim** (2026-09-15;
fixed, PR #22; **confirmed on the cabinets**: Mark played a twelve player Campaign and a twelve
player Deathmatch across the laptop, the Windows cabinet and the Pi, and both behaved correctly). In
a Campaign hosted by the Windows cabinet, a player's weapons would not auto-switch and a
player could not hit barrels. The Windows log's `LINKNAMES` showed the laptop's four players (the
last cabinet in) as `Player 9`..`Player 12` in colour 0 on every cabinet. Their `XD_NAMEANDCOLOR`
and `XD_WEAPONPREF` never ran, leaving `GF_flags` and `favoritweapon` zeroed: no original weapon
switch, no autoaim, no weapon order. Mark's duplicated player names were a red herring; nothing in
the engine checks for duplicates.
- **Cause.** A joining node sends its players' config from `Got_NetXCmd_AddPlayer`, the moment it
  runs their add. A client can be running tics the server has made but not yet run itself
  (instrumented: sent at `gametic=2070`, arrived at the server at `gametic=2070 maketic=2075`).
  `net_textcmd_handler` refused textcmds for players not in `playeringame`, so all four were thrown
  away. `D_Send_PlayerConfig` runs again whenever anyone joins, which rescued every cabinet but the
  last one in. The host's own players hit the same refusal at game start and were rescued the same
  way. It is timing: a joiner locked in passed; one still choosing when the countdown ran out
  failed 2 runs in 3.
- **Fix.** On the server, intake also accepts a player already committed to the sending node
  (`player_to_nnode[pn] == nnode`, set by `SV_commit_player` before the add runs). The old
  commented-out "ideal test" was exactly this, avoided because bots do not set it up, so it is an
  addition to the old test, not a replacement. `ExtraDataTicker` still requires `playeringame`
  when the command runs, which is in a tic after the add.
- **Second bug, same trace.** `SV_ResetServer` cleared `playeringame` and `player_to_nnode` but not
  `localplayer[]`. A cabinet joining its second linked game announced a config for its player
  number from the first game, which by then belonged to another cabinet: the laptop sent
  `pn=4` as its players joined game two, and player 4 was the Pi's. It could rename, recolour or
  re-flag somebody else's player. `SV_ResetServer` now calls `CL_Init_localplayer`; every caller
  runs it before players are added.
- **`LINKNAMES`** now prints `name/colour/flags/weapon order` per player (`oa/045628137`; missing
  shows `--/_________`). The case also showed the cabinets *disagreeing*: the Pi had those
  players' autoaim on while the host and laptop had it off, which is a desync, not only a missing
  name.
- **Cases:**
  - `names12`: 12 players, the master hosts.
  - `names12memberhost`: a member hosts, as the Windows cabinet did.
  - `rejoin12`: the session as played, with hooks `-linkhostagain` and `-linkjoinpanels2`. The
    host hosts twice; the laptop joins with one player then four, pressed in but not locked; the
    Pi joins only the second game. It failed with the exact picture from the cabinet, and
    passes 5 runs in 5 with the fix.

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

## Music Cabinet

Music does not survive being played on several machines at once. Nothing keeps the tracks in step:
each cabinet starts its own at its own moment and they drift within a few bars. Syncing them properly
is not available with what is to hand — SDL_mixer exposes no sample-accurate playback clock, and a
streamed track has its own timebase — and **starting** them together only delays the drift. Real
linked arcade hardware does not phase-lock audio either; it designates whose speakers carry it.

`cv_link_musiccab` ("link_musiccab", **Arcade Options → Cabinet Link Options → Music Cabinet**,
`CV_SAVE`, default "All"). Values are cabinet **names**: "All", then this cabinet's own name, then
every pinned peer.

- **The list is built from the pins, not from the peers that happen to be online.**
  `LK_Known_Names` (`d_link.c`) reads `lk_set.name` and `lk_shared.pins`, which come from
  `legacyhome/link/pins.txt`. That matters for *when*, not just for completeness — see below.
- **`M_Link_Music_Build_List` is called from `D_DoomMain` immediately after `LK_Init` and therefore
  before `M_LoadConfig`.** A `CV_SAVE` cvar stores its *label*, and a label that is not in the
  `PossibleValue` list when the config loads is refused and the cvar falls back to its default. Built
  after the config, the operator's choice would have been silently reset on every restart, and
  everything would still have looked fine on the machine where it was set. LK_Init at d_main.c:4297,
  M_LoadConfig at 4593 — that ordering is the whole reason this can be an ordinary saved cvar.
- **Muted only during a linked game.** `M_Link_Music_Muted` (`m_menu.c`) is false unless
  `LK_In_Linked_Game()`, which is `lku_mode != LKU_NONE` — the sealed UDP game channel, up only while
  cabinets are actually playing together. A cabinet on its own, in Single Level, or on the attract
  screen keeps its music whatever the setting says. It is a "who carries the music" setting, not a
  "mute this cabinet" switch.
- **Applied in `S_Update_Volumes` (`s_sound.c`)**, the one place the mixer volumes are set, which
  D_DoomLoop already calls every pass. So it follows the attract scaling and the volume cvars without
  a second mechanism, and coming out of a linked game restores the music by itself.
- **Only the master has the page, so the master broadcasts the pick.** Cabinet Link Options is a
  master-only row (`M_Draw_ArcadeOptions`), so a member is never given this by hand — its own
  `cv_link_musiccab` sits at the default forever. Reading the local cvar on every cabinet was the
  first version's bug and it looked exactly like the feature not working: the operator set the
  master to "laptop", and the desktop, a member, carried on playing its own music because as far as
  it knew nothing had been chosen. `LKG_Ticker` on the master sends `LK_GM_MUSIC_CAB` (a cabinet
  name) to every cabinet every four seconds, and `LKG_Music_Choice` returns that on a member and the
  master's own cvar on the master.
  - **Repeated rather than sent on change**, so a cabinet switched off while the setting was changed,
    or paired later, is told without anyone opening the menu. A name every four seconds is nothing
    beside the presence traffic already on the link.
  - A member that has not heard yet falls back to its own cvar — "All" unless somebody set it in an
    operator session — so the failure mode is every cabinet playing, which is the behaviour from
    before the setting existed, rather than silence.
- **The pick is a preference, with the host as the fallback** (`LKG_Music_Here`, `d_linkgame.c`).
  The chosen cabinet need not be in every game: two cabinets at one end can play each other while
  the middle one sits on its attract screen, and a rule of "only the pick plays" would leave that
  game silent — which is worse than the drift the setting exists to avoid. So: **the chosen cabinet
  carries the music when it is in the game, otherwise the cabinet that started it does.**
  - It needs nothing sent between cabinets. Any cabinet can tell whether it is itself the pick;
    only the host knows who actually joined (`lkg_remotes[]`), so only the host takes the fallback —
    and there is always exactly one host. A member that is not the pick simply stays quiet, and if
    the pick is absent the host claims it.
  - That leaves both cases whole: pick present (as host or member) → the pick carries it and the
    host is quiet unless it is also the pick; pick absent → the host carries it. Exactly one
    carrier, never none.
- Sound effects are untouched: they belong on the cabinet they happen on.

Verified by forcing the linked-game and host conditions and reading the result on all six cases:

| case | pick | result |
| --- | --- | --- |
| member | All | plays |
| member | itself | plays |
| member | another cabinet | **muted** |
| host | a cabinet not in the game | **plays** (the fallback) |
| host | itself | plays |
| no linked game at all | another cabinet | plays |

### The page stopped explaining itself

`M_Draw_LinkOptions` used to print a paragraph under each setting. Three settings needing three
paragraphs of on-screen text said more about the page than about the settings, and 272 units of menu
font is a poor place to write a sentence. They are in `README.md` now, where the operator reading the
manual gets better prose than the page could hold.

The "THIS CABINET IS NOT THE MASTER" line stays. It is not an explanation of a setting — it is the
reason none of them will do anything, which is worth saying where the settings are.

### The mute pauses the music; it must never set the volume to 0

The first version muted by setting the music volume to 0, and on the Windows cabinet that **took the
sound effects with it**. That machine was completely silent in every linked game it joined --
effects as well as music -- and came back the moment the game ended.

Every engine number said the sound was working, because it was. `-volog` measured it: effects volume
at full (`sfx=24`), 52 sounds started, nothing refused (`nochan=0 nodata=0`), the mixer reading those
channels every buffer (`mixchan` climbing), and the post-mix callback never missing one (`mixcalls`
climbing by ~43 in every report, right through the silence). The loss was underneath all of it.

**SDL_mixer plays MIDI on Windows through the system synth (winmm), where `Mix_VolumeMusic()` lands
on `midiOutSetVolume()` -- which attenuates the program's whole audio output, not the MIDI stream
alone.** Zero there is zero for everything the process plays. Linux mixes MIDI into the same buffer
as the sound effects and never goes near that call, which is why it reproduces on no machine here:
`musicvolume "0"` on Linux leaves `sfxpeak` at a healthy 12593.

Pausing touches no volume control on any platform. If a backend ever ignores the pause, the failure
is music playing on two cabinets at once -- the complaint this feature started from -- rather than a
cabinet with no sound. The pause is **re-asserted every tic** rather than set once on the edge:
`S_ChangeMusic` runs at every level change and `I_PlaySong` knows nothing about this mute, so a
one-shot pause would be undone at every map.

`tools/linktest.sh musiccab` guards it. The symptom cannot be reproduced on Linux, so the case
checks the **mechanism**: while the mute is active, the music volume the engine asks for must not be
0. Shown to go red by reinstating `want_mus = 0`, which is the change it exists to catch.

**The general lesson, and it is the expensive one here:** four rounds of diagnosis were spent below
the engine because `sfx=24` was read as proof that the sound path was innocent. It was proof that
the *engine* was innocent, which is not the same thing -- a volume the engine never touches can
still be turned down underneath it, by a call that appears to be about something else entirely. And
the search was steered for three of those rounds by a report that the cabinet had sound when all
three were joined, which turned out to be a neighbouring machine's speakers. **Check which box the
sound is coming out of before building a theory on it.**

### Tracing a silent cabinet: `-volog`

A cabinet that joined a linked game hosted elsewhere went **completely** silent — sound effects as
well as music — and came back the moment the game ended. Music Cabinet was the obvious suspect and
was wrong, which is the whole reason this switch exists: four hypotheses were formed and killed in
turn by reading the code, and the only thing that moved the diagnosis was numbers off the machine
that does it.

`-volog` prints a line whenever something changes, and **writes `volog.txt` beside the program**.
The file is not a nicety. On Windows the program is a GUI binary with no console attached, and
`LOGMESSAGES` is commented out of a normal build (`doomdef.h`), so a diagnostic that only calls
`GenPrintf` is **invisible on the one machine that shows the fault**. That cost a whole round trip.
Flushed per line, so a cabinet switched off at the wall still leaves what it had.

Two kinds of line. The first is what the mixer was set to and what decided it:

```
VOLOG sfx=24 mus=0  cv_sfx=24 cv_mus=7 cv_attract=50 attract=0 menuover=0 musiccab_muted=1 netgame=1 gamestate=6
```

The second is the whole chain from "a sound was asked for" to "samples were mixed into the buffer
handed to the audio device":

```
VOLOG sounds asked=52 inaud=8 nochan=0 nodata=0 started=44 mixchan=1061 peakvol=20/20 mixcalls=810 ...
```

| field | meaning | where it is counted |
| --- | --- | --- |
| `asked` | requests, however they end | top of `S_StartSoundAtVolume`, **before** the `nosoundfx` early-out |
| `inaud` | dropped as out of earshot | the `!audible1` branch |
| `nochan` | `S_get_channel` refused | |
| `nodata` | the lump has no samples | |
| `started` | published to the mixer | end of `I_StartSound` (`sdl/i_sound.c`) |
| `mixchan` | channel-passes the mixer read samples from | `I_UpdateSound_sdl`, per buffer |
| `peakvol` | loudest left/right volume started since the last report, 0..127 | `I_StartSound` |
| `sfxpeak` | loudest sound-effect **sample actually written**, 0..32767 | `I_UpdateSound_sdl` |
| `mixcalls` | post-mix callback invocations | `I_UpdateSound_sdl` |

A single `VOLOG audio` line is written alongside the first of these, naming what SDL_mixer actually
opened — driver, rate, sample format and buffer — taken after `Mix_QuerySpec`, so it is what the
device gave rather than what was asked for. The format matters: the mixer writes `Sint16`, and a
device that came back as float would turn every sample into a denormal and play silence.

Reading it:

- **`sfxpeak` healthy** — real audio reached the buffer SDL sends to the device. Everything in the
  engine worked and the silence is below it: the device, its volume, or which device was opened.
  This is the field that ends the search; the rest describe intent, and intent is not output.
- **`sfxpeak` at 0 while `started` and `mixchan` climb** — the channels are being read but the
  samples in them are silent, which is a different bug entirely and points at the sfx lump data
  (`S_FreeSfx` is called for every replaced `DS*` lump when a wad is added, and joining a game
  adds the host's wads).
- **`started` climbing, `mixchan` climbing, `peakvol` non-zero** — audible samples went into the
  buffer handed to SDL. The engine is done; the fault is the device or the OS.
- **`started` climbing, `mixchan` flat** — the mixer never sees the channels.
- **`mixcalls` flat** — SDL_mixer stopped driving the device.
- **a gap between `asked` and `started`** — one of `inaud`/`nochan`/`nodata` names which rejection
  is eating them.

Two traps this encodes:

- **`asked` minus `inaud` is not "accepted"**, and reading it that way overstated one round's
  conclusion. `S_get_channel` can refuse and a zero-length lump is dropped later still, which is
  why `nochan` and `nodata` exist as their own columns.
- **`mixchan` alone does not prove sound was audible.** A channel started at volume 0 is still read
  by the mixer every buffer; it mixes silence. Set `soundvolume "0"` and `mixchan` climbs to within
  a few counts of a normal run while `peakvol` reads `0/0`. The two fields have to be read together.

`peakvol` is a **peak over the reporting window, not the latest value**, because a single snapshot
lands on whatever sound happened to be last and one genuinely distant shot reads `0/0` while the
cabinet is perfectly audible. That false alarm appeared in the first version of the field and is
why it is a maximum now.

`sfxpeak` costs a subtract, an absolute value and a compare per sample in the mixer's inner loop.
That loop runs about 21 times a second over 1024 samples, so this is roughly 44k integer operations
per second — worth stating rather than waving away, but three orders of magnitude below anything
that shows up, and unlike the renderer's drawers this loop is not on the frame path at all.

Each column was shown to go red before being trusted: `-nosound` holds `started` at 0 while `asked`
climbs to 89, and `soundvolume "0"` pins `peakvol` at `0/0` with everything else unchanged.

The counters are always compiled in — a few increments — but nothing is printed or written without
`-volog`.


## Team Deathmatch over the link

Invites carry a category byte, and Team Deathmatch is a third one, **`LKG_CAT_TEAMDM`**, appended
after Campaign. `lkg_on_invite` drops any category it does not know, so a cabinet built before it
ignores a Team Deathmatch invite rather than opening a plain Deathmatch join screen. The invited
cabinet's join screen reads the category through **`LKG_Category()`** in `M_Join_Remote_Open` and
becomes a team one (TEAM row, four colours) — which is the *only* thing that puts the remote
players on teams, since each cabinet's players bring their own colour. `teamplay` and `teamdamage`
themselves are `CV_NETVAR`s and arrive from the host with the rest. `-linkautohost teamdm` hosts
one; `tools/linktest.sh teamdm` plays four a side and checks every player's colour and both rules on
both cabinets.

## Tracing an unexplained PAUSE

A linked game that shows **PAUSE** with nobody having pressed it is almost always the server's
*network wait* (`SV_network_wait_timer`, `d_clisrv.c`), not a real pause: it draws the same
`M_PAUSE` graphic and is pushed to every cabinet with `SV_Send_State`. The server raises it when a
cabinet has drifted out of step and asks for a **player repair** (`RQ_REQ_PLAYER`, 18 tics) or is
sent a **savegame** (90 tics). A cheap repair the server offers on its own (`SV_Send_player_repair`
from `SV_consistency_fault`) does *not* pause. Drift is found by comparing each client's per-tic
`Consistency()` (player x positions plus the `P_Random` index) with the server's.

First seen in Team Deathmatch across three cabinets, alongside **SCORES: DIFFERENT BUILD** on the
Cabinet Link page. That message is shared scores only, but it means the cabinets' binaries come
from different commits (`LK_Build` is the `DLA_VERSION` suffix of the banner), and any gameplay
difference between them drifts the simulation. Two cabinets on the *same* build ran
`CAT8=teamdm linktest.sh chaos8` (eight players on random buttons, 10% loss) with no fault.

**Resolved (2026-09-16) by putting all three cabinets on one build.** Once the laptop, the Pi and
the Windows desktop were all on `09a3bf1` and rebuilt, Team Deathmatch played without the pauses.
No `-logfile` run caught a failure, so this is known from the fix, not from a trace: the
mismatched builds were the cause. The mismatch came from how the code reached the machines, not
from anything in the game:

- PR #26 was merged on GitHub while a further commit was still being pushed to its branch. The
  laptop was on that branch's last commit, the Pi and the desktop on the GitHub merge, so the
  builds really differed.
- The desktop had earlier run `git merge origin/<branch>` itself, which left a local merge commit
  with no changes in it. Its `main` then had a commit GitHub lacked, and `git pull --ff-only`
  refused until `git reset --hard origin/main`.
- **Identical code on different commits is still "DIFFERENT BUILD"**: the build id is
  `git describe --tags --always --dirty`, so the hash is part of it. A laptop on a branch's last
  commit and a Pi on the GitHub merge of that branch differ even when their files match.

The rule since then: changes arrive only as a GitHub PR merge, and every cabinet updates with
`git checkout main && git pull --ff-only` and then rebuilds; `git rev-parse --short HEAD` must print
the same hash on all of them. The `NETTRACE` lines stay in, for the next time a linked game pauses
by itself.

**`NETTRACE` lines** (`EMSG_errlog`, so the terminal and `-logfile`, never the console):

| line | where | says |
| --- | --- | --- |
| `snap tic tic=N` | every 175 gametics, every cabinet (`NT_Snapshot`) | server, paused, teamplay, teamdamage, P_Random index, consistency, hits the team rule blocked (`nettrace_blocked_hits`, `P_DamageMobj`), and per player `colour/health/x,y/frags` |
| `consfault` | server, `SV_consistency_fault`, now whatever `verbose` says | the drifting node, its players by name, server and client values, fault count |
| `snap at-fault` / `conshist server` | with it | the server's state, and its last 32 consistency values as `tic:value` |
| `repair received` / `snap before-repair` / `conshist client` | client, `repair_handler_client` | the same from the drifting cabinet, before the repair overwrites it |
| `waitpause ... why=` / `waitpause end` | server | the network-wait pause and its cause (savegame, player repair, bot seed) |
| `statepause` | client | a pause state the server sent |
| `pause` / `pausecmd` | `Got_NetXCmd_Pause` / `Command_Pause` | a real pause, and who asked |
| `color` | `Got_NetXCmd_NameColor` | a player's colour changing, i.e. their team |

**`tools/nettrace-diff.py host.txt pi.txt win.txt`** lines the logs up: each cabinet's build and
events, a warning if the builds differ, the **exact first tic** the consistency histories differ
(from the `conshist` lines two cabinets left around a fault), and the first 5 s snapshot that
differs, field by field. The snapshots alone can miss a drift: a repair fixes the game before the
next one, which is why the histories are dumped at the fault.

Verified by forcing it: a temporary extra `P_Random()` on the client at gametic 700, in a
`CAT8=teamdm chaos8` run, produced `consfault` at 702 and 703 with the player names, both
cabinets' histories, and `nettrace-diff.py` reported "consistency first differs at tic 701" — while
the snapshots, taken after the repair, all agreed. Also found on the way: `RQ_CLOSE_ACK` runs
`SV_network_wait_handler` with no pause on, so the `waitpause end` line prints only when one was.
