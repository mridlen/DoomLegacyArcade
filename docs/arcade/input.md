# Panels, gamepads, control schemes and menu key translation

*Part of the DoomLegacy arcade cabinet build. Read before touching `g_input.c`, the joystick/axis handling in `sdl/i_system.c`, `gamecontrol_pl[]`, the guided setup, or `M_Cabinet_Menu_Key`/`M_key_is_control` in `m_menu.c`.*

See `CLAUDE.md` for the build, headless verification and the cross-cutting rules index.

---

- **The menus are driven by the cabinet buttons** (`m_menu.c`, `M_Cabinet_Menu_Key`, called from
  `M_Responder`'s `ev_keydown`). The panel has no arrow keys, Enter or Escape, so both players'
  buttons are translated: forward/backward = cursor up/down, turn *or* strafe left/right =
  left/right, fire = select, use/open = back out. Read from `gamecontrol[]`/`gamecontrol2[]` **by
  action**, not by hardcoded character, so it follows the selected control scheme and any
  rebinding. Turn and strafe both map to left/right on purpose: the two schemes only swap which
  side pair is turning, so mapping both makes the `,aoe` diamond behave identically either way.
  Only applied while `menuactive` — otherwise "use" would open the menu during play instead of
  opening doors. Note the menu is still *opened* with Escape, which is not on the panel.
  - **Applied in devmode too, with text entry excluded.** There is no longer a devmode exemption:
    a leverless/hitbox panel is *just a keyboard*, so exempting keyboard keys left such a panel
    unable to work the menus while an operator was configuring it. The guard is now on the focused
    item being an **`IT_KEYHANDLER`** (the name and skin fields), because that dispatch happens far
    later in `M_Responder` — without it, "use" on `a` would exit the menu instead of typing an A.
    The cost is that in devmode a letter bound to a control no longer reaches the letter-shortcut
    search, since the translation claims it first.
  - Skipping joystick keys in devmode had left the panel at the mercy of **upstream's hardcoded
    `KEY_JOY0BUT0` → `KEY_ENTER` and `KEY_JOY0BUT1` → `KEY_BACKSPACE`** (`M_Responder`'s
    virtual-key `switch`), which selects and cancels by button *index*.
  - That upstream mapping is why **a stick's mode switch changed which buttons work the menus**. On
    a Mayflash F300, DirectInput/PS3 happened to put fire and use on buttons 0 and 1 and so looked
    correct, while XInput put A and B there — so A selected, B cancelled, and the operator's own
    fire button did nothing. Nothing was wrong with the bindings; the hardcoded indices simply
    pointed at different physical buttons.
  - **Bindings are stored by button index, so they do not survive that toggle.** Switching between
    XInput and DirectInput renumbers the buttons, and every `setcontrol`/guided-setup binding then
    refers to a different physical button. Pick a mode and re-run the guided setup in it.
  - `M_key_is_control` checks `gamecontrol[]` *and* `gamecontrol2[]`, so either player's panel
    drives the menus.
- **Menu letter shortcuts are disabled** outside devmode (`m_menu.c`, `M_Responder`'s `default:`
  case returns before the `alphaKey` search). The cabinet is buttons-only and several of those
  buttons are letters that collided with the shortcuts — player 1's turn-right button (`e`) on the
  New Game menu jumped to END GAME, one Enter from ending the run now that prompts are gone. Done
  at the dispatch point rather than by clearing `alphaKey` per menu, so it covers every menu.
  Text entry is unaffected *by this*, because the `IT_KEYHANDLER` dispatch (line ~6582) comes
  before the `default:` case. **Note the dispatch is not early in `M_Responder` generally** — an
  earlier version of this file said `IT_KEYHANDLER` items "consume the key earlier", which is only
  true relative to the letter shortcuts. Anything acting on the key *above* line 6582 sees it
  first, which is exactly the trap the cabinet menu translation had to be guarded against.
- **Screenshots** are on **F12** (`gc_screenshot`), with PrtSc as the second binding. The stock
  default was SysRq (Alt+PrtSc), a modifier combination; and **PrtSc itself never reaches the game
  under GNOME**, whose own screenshot tool takes it first. F12 costs the engine's hardcoded "spy
  mode" on that key (`G_Responder`) — `M_Responder` consumes the screenshot key first — which is
  single-player-only and of no use on a cabinet.
  - **`M_Responder` only tested `gamecontrol[gc_screenshot][0]`**, so a second key assigned to
    Screenshot silently did nothing. It checks both slots now.
  - **`KEY_PRINT` had no entry in the key name table**, so it could not be bound or saved by
    `setcontrol` at all; it is `"print"` now. Changing the compiled default is not enough on its
    own — `config.cfg` carries a `setcontrol "screenshot"` line that is loaded afterwards and wins,
    so both the tracked and live configs were updated to
    `setcontrol "screenshot" "print" "sysreq"`.
  - Written to the **current working directory** as `DOOM0000.tga`, counting up to the first
    unused number (`HRTC`/`CHXQ` for Heretic/Chex). Targa because the cabinet runs OpenGL; the
    software renderer writes `.pcx`. `screenshotdir` redirects it, and only then is the file named
    after the first loaded PWAD instead of the game.

- **Analog joystick axes are translated to hat keys** (`sdl/i_system.c`, the `SDL_JOYAXISMOTION`
  case). **See also the gamepad section below**, which fixes the event routing this depends
  on: the `.which` these handlers index by is an SDL2 *instance id*, not a slot index, and
  `previous_jhat` used to be shared by every joystick.
  Upstream produced **no bindable input from any analog axis**: the only handling was for
  Xbox triggers, gated on `check_Joystick_Xbox[]`, which requires the joystick's *name* to match
  one of two literal strings (`"Xbox 360 Wireless Receiver (XBOX)"` / `"Microsoft X-Box 360 pad"`)
  and covers only axes 2 and 5. An arcade stick in analog mode was therefore completely dead, while
  the same stick on its d-pad setting worked, because `SDL_JOYHATMOTION` *is* translated
  generically. Diagnosed on a Mayflash F300, which enumerates as `Generic X-Box pad` — 6 axes,
  11 buttons, 1 hat — and so fails that name test.
  - Axes 0 and 1 now emit the **same `KEY_JOY0HAT*` codes as the hat**, via the existing
    `Translate_Joyhat`. That is the point: analog and d-pad modes become interchangeable and every
    existing binding keeps working. Emitting *new* key codes would have needed rebinding.
  - **It has to be key events, not `bindjoyaxis`.** The engine does have an analog axis path
    (`bindjoyaxis`, `joybinding_t`, `ja_move`/`ja_turn`/`ja_strafe`/`ja_pitch`), but those drive
    movement only. Menu navigation (`M_Cabinet_Menu_Key`) reads `gamecontrol[]` *by key*, so an
    analog-bound stick could never move the menu cursor.
  - The `case` was moved **out of** `#ifdef XBOX_CONTROLLER`. It was previously inside it, so
    without that define there was no `SDL_JOYAXISMOTION` case at all.
  - `previous_jaxis[joystick][axis]` latches the current direction so jitter inside a direction
    does not re-post keydowns, and a straight left-to-right flick releases the old direction before
    pressing the new one. Verified against a synthetic trace (jitter, flick, diagonal, deadzone):
    no repeated keydowns, no orphan keyups, nothing left stuck.
  - Deadzone is half range (`JOYAXIS_DEADZONE` 16384). An arcade stick reports full deflection, so
    this only has to clear noise; it also stops a diagonal registering until genuinely committed.
  - Only axes 0/1. Axes 2 and 5 are the triggers, handled above and `break`ing early so a trigger
    is never read as a direction; the right stick (3/4) is left alone.
  - **The triggers had the same disease and the same cure.** `LT`/`RT` arrive as axes 2 and 5, and
    their handler was gated on that identical `check_Joystick_Xbox[]` name test, so they were dead
    on the F300 too. The gate is gone; the axes are read on any joystick. That is safe because
    `KEY_JOY0LEFTTRIGGER`/`RIGHTTRIGGER` are distinct codes, unbound by default, and already have
    `setcontrol` names (`Joy0 lt`, `Joy0 rt`) — so a pad using those axes for something else costs
    nothing unless a player binds them.
    - Threshold is `> 0`, **not** `JOYAXIS_DEADZONE`: triggers rest at full *negative* on the Linux
      `xpad` driver and at zero on others, so positive means pressed under both conventions.
    - `previous_jtrigger[joystick][2]` latches the pressed state. Upstream posted a keydown on
      *every* axis event while a trigger was held past the threshold, which is a stream of them for
      an analog trigger; now only transitions post. Verified against both resting conventions, with
      LT and RT independent.
    - `check_Joystick_Xbox[]` is now **assigned but never read**. Left in place (it is upstream
      code, and harmless) rather than removed.
- **Control schemes** (`g_input.c`). `cv_controlscheme[2]` ("Look and Move" vs "WASD") per player,
  selectable on the Setup Player 1 and 2 screens. `ControlScheme_Apply()` owns ten bindings per
  player (move/turn/strafe/fire/use/weapon cycling); everything else is left alone. The two schemes
  differ **only** in which pair turns and which strafes — `pair_a` turns under "Look and Move" and
  strafes under "WASD", `pair_b` the reverse. The built-in presets' keys are the characters the
  user's **Dvorak** layout produces — the engine captures layout-aware SDL keycodes, not physical
  scancodes (`sdl/i_system.c`).
  - `ControlScheme_Apply` is driven **only** by the cvar's `CV_CALL` OnChange — there is no other
    caller. It therefore fires *during* config load, at the `controlscheme` line, which
    `config.cfg` writes well **before** the `setcontrol` lines. So the `setcontrol` lines execute
    last and win: hand-edited bindings *do* survive a load. (An earlier version of this file
    claimed they would not stick; a headless test with `setcontrol "forward" "z"` showed `z`
    surviving under **both** presets.) What actually loses them is *toggling the scheme cvar*,
    which re-stamps all ten immediately.
- **Guided control setup** (`m_menu.c`, `M_Guided_Controls_P1`/`_P2`, under Options → Setup
  Controls, which is devmode-only). Prompts for the ten controls a cabinet panel needs, binding
  each to whatever is pressed — a 4-way stick plus six buttons. Everything else stays on the
  ordinary Setup Controls pages. Built on the same `MM_EVENTHANDLER` message plumbing as
  `M_ChangeControl`, so the message box, key capture and event routing are shared; mouse and
  joystick buttons arrive as `ev_keydown` with codes in the key space, so any panel wiring works.
  - Order is **stick first, then the buttons by number**, each labelled with it:
    `STICK UP / DOWN / LEFT / RIGHT`, then `BUTTON 1 - FIRE`, `BUTTON 2 - STRAFE LEFT`,
    `BUTTON 3 - STRAFE RIGHT`, `BUTTON 4 - USE / OPEN`, `BUTTON 5 - WEAPON DOWN`,
    `BUTTON 6 - WEAPON UP`. **Keep this in step with the recommended layout page** — the two are
    meant to be read together, and an earlier version asked in a different order (strafe before
    fire) while the page claimed otherwise. The numbers are only the recommendation; a panel wired
    differently still works, the operator just presses what they want for that action.
  - **Recommended layout screen** (`M_Draw_RecLayout` / `RecLayoutDef`, same menu). An
    informational page showing the play-tested assignment on the standard six-button arrangement
    (1 2 3 top row, 4 5 6 bottom): 1 fire, 2/3 strafe left/right, 4 use/open, 5 weapon down,
    6 weapon up, stick moves and turns. Modelled on the "Read This" screens — a custom
    `drawroutine` plus one `IT_SUBMENU | IT_NOTHING` item that backs out.
  - **The guided setup opens on that page**, showing "PRESS ANY BUTTON TO BEGIN", so the operator
    sees the target layout in the order they are about to be asked for it. `guided_intro` gates
    the footer text, and `M_Responder` consumes the next `ev_keydown` **before** the generic menu
    handling, so the page's own invisible item cannot swallow the press; ESC falls through to the
    normal back-out. It has to be a separate screen rather than something drawn behind the
    prompts, because `M_StartMessage` makes `MessageDef` the current menu.
  - Both wizard exits (finished, and ESC part way) call `M_Guided_Leave_Intro_Page()` to
    `Pop_Menu()` that page off the stack first. `M_StartMessage` records
    `message_menu_back = currentMenu` and `M_StopMessage` returns there, so without the pop the
    closing "Controls saved" message drops the operator back onto the layout diagram instead of
    the controls menu. **Any flow that opens a page and then raises messages has this same
    trap.**
  - **`hu_font` is proportional and space is only 4px**, so monospace ASCII art does not line up:
    `V_DrawString` advances by each character patch's own width. The layout screen therefore
    positions every element at an explicit x (`M_Centre_At`) instead of padding with spaces. Only
    `'!'`..`'_'` exist and lowercase folds to uppercase — there is **no `|`** — which is why the
    stick's down arrow is the letter `V`. Extents were checked against the real `STCFN0xx` widths
    read out of the IWAD rather than estimated; the widest line is 214px of 320. Re-check with a
    script that reads those lumps if any string here changes — do not eyeball it.
  - The three new entries sit at the **top** of `MControlMenu`, which is safe only because nothing
    indexes that menu by position and `MControlDef.lastOn` is never assigned (verified) — so the
    cursor lands on "Guided setup P1", which is the point. The rest of the lockdown does address
    menu items by hardcoded index, so this is the exception, not the pattern.
  - The prompts deliberately name the **physical control, not the game action**. They first said
    "TURN LEFT" with a note that WASD mode swaps turn and strafe, which reads as confusing to
    someone actually wiring a panel: they know they are pushing the stick left and should not have
    to reason about the simulation to answer. Keep it that way.
  - Finishing does **not** write bindings directly. It hands the ten captured keys to
    `G_Save_CustomControls()`, which stores them in **`cv_customcontrols[2]`** — a `CV_SAVE` string
    cvar per player holding ten space-separated key codes in `CK_*` order. `ControlScheme_Apply`
    prefers that table over the compiled-in `scheme_keys[]` whenever it parses, so **the cabinet's
    control layout lives in `config.cfg`** and the hardcoded table is only the fallback preset.
    An empty or malformed string falls back silently.
  - Going through the scheme machinery instead of writing `gamecontrol[]` is what keeps
    **"Look and Move" / "WASD" working on a custom panel**. The stick's left/right lands in pair A
    and the strafe buttons in pair B, which is the Look-and-Move arrangement; picking WASD
    afterwards swaps which pair turns and which strafes. The operator is never asked about this —
    it is a player preference applied after the fact. Verified headless with a distinct key per
    slot: under "Look and Move" turn was `a`/`e` and strafe `t`/`n`; under "WASD" they traded,
    with forward and fire unmoved.
  - `cv_customcontrols` is `CV_CALL` onto the same OnChange as the scheme cvar, so whichever of the
    two lines config.cfg happens to list last still leaves the bindings correct.
  - Only `ev_keydown` is accepted. Taking `ev_keyup` too would let the release of one press land
    on the next prompt and bind two actions to the same button.
  - ESC abandons the whole table rather than keeping a partial one — a half-taught panel is worse
    than the layout that was already working.
- **Xbox-style gamepads, one per panel** (`g_input.c`, `m_menu.c`, `sdl/i_system.c`). A four panel
  cabinet can be driven by four gamepads instead of (or beside) wired arcade panels.

  **Most of this already existed and nobody had used it.** `keys.h` has always given all four
  joysticks their own button, hat and trigger key codes (`KEY_JOY0BUT0`..`KEY_JOY3BUT15`,
  `KEY_JOY*HAT*`, `KEY_JOY*LEFTTRIGGER/RIGHTTRIGGER`), `keynames[]` in `g_input.c` names every one
  of them (`"Joy2 b4"`, `"Joy3 hl"`, `"Joy1 rt"`) so `setcontrol` persists them, and
  `gamecontrol_pl[panel]` is per panel. So *a pad has always been bindable* -- run the guided setup
  for panel 3 and press its buttons. What was missing was a way to say "this pad drives panel 3"
  without ten prompts, and four bugs that only appear once there is more than one pad.

  - **`.which` is an SDL2 *instance id*, not a slot index -- this is the one that matters.**
    `Translate_Joybutton(Uint8 which, ...)` took the event's `.which` straight from SDL and clamped
    `which >= MAXJOYSTICKS` down to `MAXJOYSTICKS-1`. Under SDL2 that field is an instance id which
    increments for the life of the process, so a pad that sleeps and wakes -- or is unplugged and
    replugged -- comes back as instance 4, 5, ... and was folded onto **Joy3, on top of whatever
    real pad was already there**: two players silently sharing one identity. Four pads plugged in
    before launch happen to get ids 0..3, which is exactly why single-pad testing never saw it.
    `Joystick_Index_Of()` resolves instance to slot now, and an unknown device is ignored rather
    than folded. **SDL1's `.which` really is the device index**, so that path is unchanged and the
    resolver is `#ifdef SDL2`.
  - **Hotplug.** `I_JoystickInit` enumerated once at startup and never looked again, so a pad
    plugged in after launch was invisible for the life of the process -- which on a cabinet, where
    the pads live in a drawer and come out for a four player game, is the normal case.
    `SDL_JOYDEVICEADDED`/`REMOVED` are handled, both going through the same
    `Joystick_Open_Device`/`Joystick_Close_Slot` as startup so a pad present at boot and one
    plugged in later end up in identical state.
    - **`SDL_JOYDEVICEADDED` is queued at init for devices that were already present**, so startup
      enumeration and the hotplug handler both see the same pad. Without a dedup the one controller
      opened **twice** -- slot 0 from `I_JoystickInit`, slot 1 from the queued event -- and a single
      physical pad appeared as two joysticks with two sets of key codes. Caught headlessly on the
      first run: `1 joystick(s) found.` followed by `Joystick connected in slot 1.`
    - **`.which` is a *device index* on ADDED and an *instance id* on REMOVED.** That asymmetry is
      SDL's, and getting it backwards silently does nothing.
    - **Slots are sticky by device path** (`SDL_JoystickPathForIndex`, SDL 2.24+), so a pad that
      reconnects lands back in the slot it had rather than shuffling everyone one seat along. On a
      cabinet the pads stay in the same USB ports, which is what the path names -- and **four
      identical pads share an SDL GUID**, so there is no other way to tell them apart. Without that
      SDL version the slot is simply not sticky, which is the old behaviour.
    - Slots therefore need not be contiguous. `num_joysticks` is a *count* (`I_Joystick_Count`) for
      display and bounds, **never an index**; `I_JoystickNumAxes`/`I_JoystickGetAxis` test
      `joysticks[n]` itself, and `I_ShutdownJoystick` walks slots rather than a count.
  - **`previous_jhat` was a single global pair** (`Uint8 previous_jhat[2]`), so four pads shared one
    hat state and cleared each other's d-pad directions. It is `[MAX_JOYSTICKS][2]` now, matching
    `previous_jaxis` beside it, which had been done correctly.
  - **`M_key_is_control` only tested panels 1 and 2** (`gamecontrol`/`gamecontrol2` literally), so a
    panel 3 or 4 control -- a third arcade panel, or a pad bound to one -- could play but **could
    not work a single menu**. Same family as the six per-player cvars that stopped being registered
    at Player2: the array was widened and a hand-written site was not. It loops
    `MAXSPLITSCREENPLAYERS` now, and refuses `key == 0` so an unbound row cannot match.
  - **Analog axis bindings were keyed by local player index, not panel.** `G_BuildTiccmd` does
    `gcc = gamecontrol_pl[D_Panel_Of(pind)]` and then, twenty lines later, the joybinding loop did
    `if (jb.playnum != pind) continue`. The same views-versus-panels conflation that broke the
    player lookup itself; it asks `D_Panel_Of(pind)` now.

  **Options -> Setup Controls -> "Xbox Controllers >>"** (`m_menu.c`, `XboxMenu`/`XboxDef`) is one
  row per panel. Its job is **assignment, not binding**: press the row, press a button on the pad
  you mean, and `G_Apply_Xbox_Preset` stamps the whole layout onto that panel at once.
  - Assignment rather than a second per-device config screen, because the rest of `m_menu.c` is
    organised per *panel* -- a device-oriented page would make the operator hold both "which pad is
    Joy2" and "which panel does it drive" in their head at once. Here they never learn the joystick
    number; they press the pad in their hands.
  - **Nothing new is persisted.** The ten controls the scheme owns go through
    `G_Save_CustomControls` into `cv_customcontrols[panel]` (already `CV_SAVE`, already preferred by
    `ControlScheme_Apply` over the compiled `scheme_keys[]`), so **"Look and Move" / "WASD" keeps
    working on a pad**; the rest are ordinary `gamecontrol_pl` entries, already written out as
    `setcontrol`/`setcontrol2/3/4` lines. This is what makes the feature small.
    - It also fills the gap `scheme_keys[]` documents: there is no third or fourth *keyboard* preset
      because the Dvorak clusters would collide, but Joy2 and Joy3 have their own key codes, so for
      pads there is nothing to collide.
  - **The assignment shown is derived back out of the bindings** (`M_Xbox_Joy_Of_Panel` reads
    `gamecontrol_pl[panel][gc_fire][0]`, falling back to `gc_forward`, through `G_Joy_Num_Of_Key`),
    so the page cannot drift out of step with what the pad actually does. A pad that is bound but
    not plugged in shows `Joy2 - off`, which otherwise reads as the binding having failed.
  - **The handler gets the *raw* key, and that is load-bearing.** `M_Cabinet_Menu_Key` rewrites
    `M_Responder`'s local `key` early -- and now that `M_key_is_control` covers four panels it
    claims *more* keys than before -- but the `IT_MSGHANDLER` dispatch calls `cc_action(ev)` with
    the original event, so `ev->data1` is untranslated. Without that, pressing a pad button already
    bound to a control (the likely case) would arrive as `KEY_ENTER`. The guided setup relies on the
    same thing.
  - **Backspace on a row takes the gamepad off that panel** (`M_Xbox_Clear` /
    `G_Clear_Xbox_Preset`), which is what Backspace already does on an `IT_CONTROL` row -- the same
    `case KEY_BACKSPACE` in `M_Responder` handles both. No confirmation and no message: the row's
    value is derived from the bindings, so it changes to `none` under the cursor the instant it
    runs, which says it better than a message box would.
    - **Keyboard only, and that test is load-bearing.** `button_key` is set for any mouse or
      joystick press, and upstream remaps `KEY_JOY0BUT1` -- **B on an Xbox pad** -- straight onto
      `KEY_BACKSPACE` in the virtual-key switch far above. Without `! button_key`, pressing B on
      this page would wipe a panel's whole layout instead of backing out of the page, which is what
      B does on every other menu. Safe because clearing is an operator action and this page is
      devmode-only, so a keyboard is always present.
    - **It restores, rather than merely unbinding.** The five extras are cleared explicitly
      (`ControlScheme_Apply` never touches them, so emptying the custom table would leave them on
      the pad for ever), the ten scheme actions are cleared explicitly too, and then
      `cv_customcontrols[pind]` is emptied and `ControlScheme_Apply` called **directly** -- not left
      to the cvar's OnChange, because `CV_Set` returns early on an unchanged value and a panel whose
      table was already empty would never be re-stamped. Net effect: **panels 1 and 2 come back on
      their compiled Dvorak keyboard preset, panels 3 and 4 have none and are left unbound.** That
      is "undo the assignment", not "make this panel dead".
    - Clearing the ten explicitly is what makes panels 3 and 4 work: `ControlScheme_Apply`
      deliberately returns without touching anything when there is no custom table *and* no
      compiled preset, so emptying the cvar alone would have left the pad bindings exactly where
      they were and the row still reporting its pad.
    - Verified headlessly: panel 1 assigned then cleared reports `, o a e t n h space w v` with the
      extras null; panel 3 assigned then cleared reports every one of the fifteen as null.
  - **Assigning a pad overwrites whatever the guided setup put in that panel's
    `cv_customcontrols`**, and clearing cannot bring it back -- it falls to the compiled preset. On
    a cabinet whose panels are wired arcade controls, assigning a pad to a panel therefore discards
    that panel's hand-taught layout. Re-run the guided setup to get it back.
  - The row is **appended** to `MControlMenu`: that menu is addressed by position (`mcontrol_*`,
    used by the lockdown) and nothing indexes past `mcontrol_p4_controls`. Rows for panels the
    cabinet does not have are hidden in `M_Configure` beside the guided-setup ones. Rows 0 and 1 are
    never hidden, so the page can never become all-hidden -- which is what hard-locked the Cheats
    menu.
  - **Measured against the real `STCFN` lumps**, at `x` 60: label `"Panel 1 gamepad"` is 110px
    ending at 170, and the widest value `"Joy0 - off"` is 70px, right-justified at
    `BASEVIDWIDTH - x` = 260 and so starting at 190 -- 20px clear. **`"Panel 1 controller"` was the
    first wording and did not fit**: 133px, ending at 193, three pixels into the value. Rows are
    `IT_WHITESTRING` and step by `STRINGHEIGHT`, so the four sit at y 40..70 and the footer at 88,
    with the Backspace hint (178px, ending at 238) one `STRINGHEIGHT` below it at 98.

  **The preset**, per joystick N (`G_Apply_Xbox_Preset`):

  | control | pad | control | pad |
  | --- | --- | --- | --- |
  | move / turn | left stick or d-pad | fire | RT |
  | **strafe left / right** | **right stick** | run | LT |
  | use / open | A | jump | B |
  | weapon down / up | LB / RB | menu | Start |
  | automap | Back | scores | L3 |

  - **The left stick and the d-pad both work from one binding**, because `sdl/i_system.c` translates
    axes 0/1 into the same `KEY_JOY*HAT*` codes the hat emits. **The right stick has its own key
    codes**, which is the whole reason it can do a different job -- see the right-stick section
    below. Everything is digital, deliberately: it feeds the ordinary key bindings, so it works with
    the menus, the guided setup and `setcontrol` exactly as every other control does, and true
    analog movement would double-apply against these same bindings.
  - Strafe sits on the right stick rather than the shoulders, which is what pushed the weapon keys
    onto LB/RB and left **X and Y free** for the operator. The strafe pair goes in through
    `CK_pair_b`, so the "Look and Move" / "WASD" selector still swaps it with turning -- on a pad
    that means WASD makes the **right stick turn and the left stick strafe**.
    - **That is a feature, and both layouts are play-tested.** It fell out of reusing the scheme
      machinery rather than being designed, so it was first written down here as a caveat; on the
      cabinet it turned out to be a good second option, the modern twin-stick arrangement sitting
      behind a selector the player can already reach. **Do not "fix" it.** Making the right stick
      always strafe would mean extending `controlkeys_t` and the `cv_customcontrols` format from ten
      keys to twelve, and would remove the better of the two layouts to do it.
  - **`gc_pause` is explicitly cleared.** Upstream binds pause to Back on Joy0
    (`G_Default_Controls`); Back is the automap here, and a cabinet nobody is watching must not be
    freezable by one player.
  - Guide and R3 are deliberately left unbound -- Guide is often taken by the desktop before it
    reaches SDL.
  - Only a `-devmode` session writes `config.cfg`, and Setup Controls is devmode-only, so assignment
    is an operator task like every other one.
  - Verified headlessly through a temporary `tmpxbox <panel> <joy>` console command: all sixteen
    controls resolve to the intended `Joy<N>` codes for panels 0 and 2 against joysticks 0 and 1,
    and `G_Joy_Num_Of_Key` reads the joystick back out of both `gc_fire` and `gc_forward`.

  **Hardware facts**, probed directly rather than guessed -- a **PowerA Xbox Series X Controller**
  enumerates as 6 axes, 11 buttons, **1 hat**, GUID `03001c62d62000000920000001010000`, path
  `/dev/input/event15`, and SDL has a GameController mapping for it:
  - **The d-pad is the hat**, so the existing generic hat translation already covers it.
  - **11 buttons is the whole pad**: 0=A 1=B 2=X 3=Y 4=LB 5=RB 6=Back 7=Start 8=Guide 9=L3 10=R3.
    The **back paddles (AGL/AGR) and the unlabelled centre button emit no button number of their
    own** -- they are firmware remaps that mirror one of these -- and the **volume rocker is a
    separate HID consumer device that never reaches SDL at all**. Do not design around any of them.
  - **Axes 2 and 5 (the triggers) rest at full negative** (-32768), which is the convention the
    existing `> 0` threshold was written for; axes 0/1 rest around 900-2600 and axes 3/4 (the right
    stick) around -400 to -650, both well inside the 16384 deadzone.

  **The right stick is a second hat** (`sdl/i_system.c`, `Translate_Joy_RStick`). Upstream read axes
  3 and 4 *nowhere at all* -- the trigger case covers 2 and 5, the stick case covers 0 and 1 -- so
  the right stick was dead input on every pad. It is translated to its own key codes, latched per
  joystick and per axis in `previous_jraxis` so the two sticks cannot clear each other, with the
  same `JOYAXIS_DEADZONE` and the same release-before-press ordering as the left stick.
  - **Its own codes, not the hat's**, so the left stick / d-pad and the right stick can be bound to
    different actions. `KEY_JOY0RSTICKUP`..`KEY_JOY3RSTICKLEFT` in `keys.h`, four per joystick
    (`JOYRSTICKDIRS`), named `"Joy0 ru"` / `"rr"` / `"rd"` / `"rl"` in `keynames[]` -- **without
    those names a key cannot be bound by `setcontrol` or written to `config.cfg` at all**, the same
    trap as the six per-player cvars that were never registered.
  - **They are appended at the very end of the `key_input_e` enum, after the `JOY_BUTTONS_DOUBLE`
    block, and that placement is load-bearing.** `cv_customcontrols` stores raw key *numbers*
    (`G_Save_CustomControls` writes `"%d %d ..."`), so a code inserted anywhere above would silently
    repoint every saved custom layout. `config.cfg`'s `setcontrol` lines store *names* and would
    have survived unharmed -- which is exactly what would have made such a bug hard to see.
    `NUMINPUTS` goes 440 -> 456; the only things sized by it are `gamekeydown`/`gamekeytapped`,
    which scale automatically, and `ev->data1` is bounds-checked against it.
  - Up and down are deliberately left unbound. A digital right stick has four directions and the
    preset only needs two.
  - Verified headlessly through the temporary `tmpxbox` command: strafe resolves to `Joy0 rl`/`rr`,
    weapons to `b4`/`b5`, and all sixteen new codes round-trip through the name table
    (`G_KeynumToString` then `G_KeyStringtoNum` returns the same number) with `G_Joy_Num_Of_Key`
    reporting the right joystick.
  - **An existing `cv_customcontrols` from before this change still parses** -- LB/RB are still
    valid key codes in the `CK_pair_b` slots -- so nothing breaks; the panel simply keeps the old
    layout until the assignment page is run again.

- **`gamecontrolname[]` was one entry out of step with `gamecontrols_e`** (`g_input.c`), and had
  been for as long as `ENABLE_COME_HERE` has been defined (it is, `doomdef.h:307`). The enum puts
  `gc_comehere` between `gc_screenshot` and `gc_menuesc`; the name table put `"comehere"` last,
  after `"automap"`. So every name from that point slipped one control along --
  `gamecontrolname[gc_menuesc]` was `"pause"`, `[gc_pause]` was `"automap"`, `[gc_automap]` was
  `"comehere"`.
  - **It survived because it round-trips.** `G_SaveKeySetting` and `Command_Setcontrol_f` both
    resolve through this one table, so a binding saved under the wrong name loaded back onto the
    right control, and both arrays were the same length so nothing ran off the end. The only
    symptom was `config.cfg` **naming the wrong control**, which matters the moment anyone reads or
    hand-edits it. The cabinet's own file recorded the upstream Xbox defaults as
    `setcontrol "pause" "Joy0 b7"` (really `gc_menuesc`, Start), `setcontrol "automap" "Joy0 b6"`
    (really `gc_pause`, Back) and `setcontrol "comehere" "Joy0 b8"` (really `gc_automap`, Guide) --
    which is how it was found, and is proof from real data rather than from inspection.
  - **Fixing it changes what an existing `config.cfg` means.** Those lines have to move up one
    control or the bindings shift on the next load. `cabinet/legacyhome/config.cfg` was updated in
    the same commit; **a live untracked `svn1749/bin/legacyhome/config.cfg` is fixed by re-saving
    from any `-devmode` session** -- which an operator assigning gamepads is doing anyway.
  - A `typedef` length check now fails the build if the two ever differ in *count*. It cannot catch
    a re-ordering, which is what actually went wrong, but it is free.

---

## Input diagnostics, and the event ring

*Added while investigating keyboard-only play through Deskflow (keyboard/mouse sharing), where
keys held down too long misbehave in a way other ports do not show. The cause was found with this
instrumentation and is written up under "What the capture showed" below; the fix is
`cv_keydebounce`.*

- **`D_PostEvent` has never checked for overflow** (`d_main.c`). `events[MAXEVENTS]` is a 64 slot
  ring and the head is advanced unconditionally, so posting 64 events between two drains laps the
  head exactly onto the tail — at which point `D_Process_Events` sees `eventtail == eventhead`,
  reads the queue as **empty**, and discards all 64 without a trace. Demonstrated with a standalone
  simulation of the same arithmetic:

  | posted between drains | processed | silently lost |
  | --- | --- | --- |
  | 63 | 63 | 0 |
  | **64** | **0** | **64** |
  | 65 | 1 | 64 |
  | 200 | 8 | 192 |

  **A lost `ev_keyup` leaves the key held as far as the game is concerned**, which is exactly what
  a stuck key looks like. This is the leading hypothesis for the Deskflow symptom, not a proven
  cause.
- **One poll drains the entire SDL queue in one go** — `I_GetEvent` is `while(SDL_PollEvent(...))`
  and `D_Process_Events` runs once per tic from `NetUpdate` (`d_clisrv.c`). So the ring is only at
  risk when more than 64 events accumulate *between* polls: a stall (level load, wipe, disk) with a
  key held, or a genuine flood. Sharing software makes both likelier — it re-injects keystrokes and
  forwards pointer motion, and network jitter batches them.
- **The engine has never looked at `key.repeat`** (`sdl/i_system.c`, `SDL_KEYDOWN`). SDL2 marks an
  auto-repeated keydown with it; DoomLegacy posts every repeat as a fresh `ev_keydown`,
  indistinguishable downstream from a real press. Ports that discard repeats generate far fewer
  events. **Do not simply filter them**: menu scrolling and console/chat text rely on repeat, so any
  filter has to apply to game events only.
- **Three candidate causes, and they need different fixes**, which is why the logging came first:
  auto-repeat flooding the ring; the ring lapping and eating a keyup; or Deskflow synthesising a
  release/press *pair* per repeat, which needs no overflow at all and would show as a keyup that
  should not be there.

### What is logged

Everything goes to **`EMSG_errlog`** — stderr and the log file, never the console — so a trace
cannot spam the screen of the session being diagnosed. Capture with
`./doomlegacy -inputlog 2> input.log`.

- **`-inputlog`** traces every key event: tic, `sym`, scancode, **`repeat`**, and the translated
  key. This is the line that answers "is it key repeat?".
- **Burst reports** (always on) fire when one poll delivers `INPUT_BURST_WARN` (24) or more events
  that actually reach the ring, with the keydown / repeat / keyup / mouse breakdown.
- **Ring lap reports** (always on, `D_PostEvent`) fire the moment the head laps the tail, naming
  the number discarded.
- Both are **rate limited to once a second** and carry a suppressed-since-last-report count.

### Two mistakes made building this, both caught by testing the detector

- **The rate limiter swallowed the first report.** `if( now - last_report >= TICRATE )` with
  `last_report` starting at 0 is false while `I_GetTime()` is itself below `TICRATE` — so the very
  first report, the one that says the condition exists at all, never printed. A forced lap at
  startup produced *nothing*. Both limiters now use an explicit `reported_once` flag. **A rate
  limiter keyed on elapsed time needs a separate "never reported" state.**
- **The burst threshold was gated on every SDL event polled**, window and system events included,
  which never reach `D_PostEvent`. Startup reported a 34 event "burst" of pure window traffic. It
  now counts only keydown + keyup + mouse, and prints the SDL total alongside, since the gap
  between the two is itself informative.
- Verified by forcing 70 events through the real `D_PostEvent` from a temporary `-inputlogtest`
  hook: both reports fired, with the lap naming 64 discarded. **The control matters as much** — an
  ordinary run with no flag prints zero `INPUTLOG` lines, so the reports mean something when they
  do appear. The temporary hook was removed before committing.

### What the capture showed

A capture of the real symptom (turning left and right until it went juttery) settled it, and
**ruled out the ring**: zero burst reports, zero lap reports, nothing dropped. Only **6 of 169**
keydowns carried `repeat=1`, so it is not SDL auto-repeat either.

What Deskflow actually sends for a held key is a complete **release-and-press pair, once every 2
tics**, each press flagged `repeat=0` and so indistinguishable from a real one:

```
t=337 KEYDOWN a repeat=0      t=339 KEYDOWN a repeat=0
t=338 KEYUP   a               t=340 KEYUP   a
```

Not a sample — that is the whole pattern: 67 of 69 down→up gaps were exactly 1 tic, and 64 of the
intervals between successive presses were exactly 2. The engine is told the key was let go and
pressed again **17 times a second**.

**Two things then go wrong, and together they are the "juttery" turning.** The key only counts as
held on every other tic, so half the input is lost; and `turnheld` (`g_game.c`, `SLOWTURNTICS`)
must count 6 straight tics before the accelerated turn rate engages, and gets reset before it ever
arrives — so on a shared keyboard the fast turn rate is *never reached at all*.

**The counts reconcile exactly, so nothing was lost.** The 6 "extra" keydowns are precisely the 6
genuine auto-repeats, which correctly carry no preceding release, and every key was up at the end
of the log. The reported "gets stuck" symptom is therefore **not** explained by this capture and
may need its own look — the nearest thing in it is two stretches where `a` stayed down for 17 and
11 tics.

### The fix: `cv_keydebounce`

**`keydebounce`** ("keydebounce", default **2**, `CV_SAVE`, range 0..6 tics, `g_input.c`). A
keyboard release is held for this many tics instead of being applied at once, and is **cancelled
outright** if the same key is pressed again inside the window — which is what a synthetic
release/press pair does. This is what X's detectable auto-repeat does for clients that ask; it is
done here because these arrive as genuine presses and nothing upstream can tell them apart.

- **Keyboard only.** The debounce applies to ids below `KEY_NUMKB`, so mouse buttons, joystick
  buttons and the cabinet panels are untouched. It lives in `G_MapEventsToControls`, which is the
  game-control funnel, so menus, the console and chat — which read events directly rather than
  through `gamekeydown[]` — are unaffected.
- **A cancelled release is not a fresh tap.** `gamekeytapped` drives impulse controls such as
  weapon switching; re-arming it 17 times a second would fire them repeatedly while the key was
  merely held. The resumed press deliberately skips it.
- **`G_Key_Debounce_Ticker` runs from the top of `G_BuildTiccmd`**, before the controls are read,
  so a release is never held past its window. It is idempotent — with four panels `G_BuildTiccmd`
  runs several times a tic and only the first does the work — and `G_Clear_Key_Debounce` drops
  pending releases wherever `gamekeydown[]` is already cleared wholesale.
- **Cost when not needed:** a real release is acted on up to 2 tics (57ms) late. `keydebounce 0`
  disables it entirely.
- **No demo desync risk, and this was checked rather than assumed.** Demos store finished
  `ticcmd_t` values and playback *replaces* the ticcmd outright (`G_ReadDemoTiccmd`, `g_game.c`),
  so local input never reaches the simulation during playback. How raw key events become a ticcmd
  cannot change an existing demo. It does change what a *new* recording contains for the same
  physical keypresses, but that is a new demo, not a desync of an old one — and the cabinet records
  from panels, not the keyboard. It is therefore **not** a cvar that needs to go in the demo
  header.

Verified by replaying the captured pattern (one event per tic, down/up alternating) through the
real event path and watching `gamekeydown['a']`:

| | `keydebounce 0` (control) | `keydebounce 2` |
| --- | --- | --- |
| while the pairs arrive | `1,0,1,0,…` every tic — the jutter | **`1` continuously** |
| after they stop | 0 | 0, two tics after the last release |

**The control is the point**: the bug was reproduced with the fix disabled before the clean result
was believed, and the release case was checked too — a debounce that never releases would be the
very bug it is meant to cure.
