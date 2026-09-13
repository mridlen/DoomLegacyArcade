# Cabinet Link: networked cabinets (PLAN — nothing here is built yet)

*Part of the DoomLegacy arcade cabinet build. This is a design plan, written 2026-09-13 before any
code. Read it before starting any of the work it describes, and replace each section with the real
write-up as the phase lands — the same way the other docs in this directory record what was tried,
what broke and how it was verified.*

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

A new `-devmode` page, **Cabinet Link**, under Setup:

- **Link**: Off / Master / Member (`cv_link_role`, default **Off** — a new switch defaults to what
  the cabinet already did).
- **Cabinet name**: shown on invites and records (`cv_link_name`, defaults to the hostname).
- **Master address** (members only), **Passcode** (entered with the keyboard, shown as `********`).
- **Allowed addresses** (master only): the member addresses it will accept. Empty means none — a
  master with no list refuses every member rather than accepting all of them.
- **This cabinet's ID** and, per peer: name, ID, state, build, last sync, and any mismatch
  (build / wads / ruleset) spelled out.
- **Sync now**, **Forget paired cabinets**.

Every row added follows the `menus.md` rule: the enum and the `choice ==` handlers move with it, and
`tools/menufit-test.py` is run.

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
replaced, and Mark has played it (finding 2). Still open: step 4 (no Windows machine was
reachable).

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
   up as server at full speed. The Pi hosted because the laptop's firewall would need opening for
   inbound UDP; a client's replies come back through without that.
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
6. **The IWAD version decides whether scores can be shared, not the game name.** The laptop had
   Doom 2 **v1.666** (`30e3c2d0…`), the Pi **v1.9** (`25e1459c…`). Mark chose v1.9 and the laptop
   was updated (old file kept in `~/games/doom-backup/`). Replaying the laptop's 18 Doom 2 record
   demos on v1.9: **16 desync at tic 1**; the two `doom2+dwango5` ones survive because the pack
   replaces MAP01. So the per-game wad fingerprint in Phase 2 is required, not a nicety — the same
   `doom2` game ID on two IWAD versions is two different games — and the netcode's existing MD5 check
   would have refused this netgame outright.

### Phase 1 — the link: identity, TLS, pairing, presence

- `HAVE_LINK` build probe in both build scripts and the `Makefile` (`LINK_OBJS`), CI package install.
- New `lk_link.c`/`lk_link.h` (thread, sockets, TLS, framing, auth), added to `MOBJS`, every line
  `// [Arcade]`.
- Key generation, `link.cfg`, pins, the address allow list, atomic writes.
- Presence and the operator page, read-only status first.
- `D_DoomLoop` polls the queue once per tic.

**Verified by:** two headless instances on the laptop pairing over loopback; then laptop and Pi.
Every rejection is shown to happen — wrong passcode, changed master key, a plain TCP client sending
garbage, an oversized frame, a replayed proof from another session, the lockout after three failures,
a connection from an address not on the allow list (closed before any TLS byte is read), a master
with an empty allow list refusing everyone
— and each of those tests is shown to go red when its check is disabled (`--selfcheck`, the rule
that a check never seen to fail is not evidence).

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

### Phase 3 — invites and linked games

- Invite banner over attract, closing an open menu for it, turning a same-game join screen into a
  join of the earlier game, remote `JOIN_STATUS`, start and timeout handling, per-cabinet
  idle/death rules.
- The UDP encryption shim, the session key, forced-off downloads, the port opened only during a game.

**Verified by:** a scripted invite between two headless instances (a `link_accept <panels>` console
command stands in for pressing fire, from the scratch `autoexec.cfg`, remembering that a command
starting a game does not start it where it appears in the script); a stranger's UDP packet — valid
Legacy packet, no tag — shown to be dropped before `HGetPacket`; an invite arriving while the
other instance has a menu open (menu closed, banner up) and while it is partway through the guided
control setup (left as a backed-out setup leaves it); both instances opening a Deathmatch in the
same tic and ending in one game with an agreed host, and a Deathmatch against a Campaign ending in
two; the latency comparison above; then
Mark plays laptop against Pi: Deathmatch, coop, starting a game on the laptop while someone is in
the Pi's menus, a cabinet that joins and then walks away, pulling the
Pi's network cable mid-game.

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
