#!/usr/bin/env python3
# Copyright (c) 2026, kyka-zavr.
"""Infinity-hyprland installer, text UI.

Keyboard first (arrows, Tab, Space, Enter), the mouse works wherever the terminal reports it: terminal emulators (xterm
mouse reporting) and the Linux console with the gpm daemon. Only the standard library is used (curses).

The UI only collects choices; the install itself is `install.sh --plain --yes ...`, run after curses is closed so that
sudo can ask for a password in a normal terminal.
"""
import curses
import locale
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# ------------------------------------------------------------------------------------------------ data

def read_layouts(path="/usr/share/X11/xkb/rules/evdev.lst"):
    """[(code, name, [(variant, name)])] from the xkb rules."""
    names, variants, section = {}, {}, ""
    try:
        with open(path, encoding="utf-8", errors="replace") as f:
            for line in f:
                line = line.rstrip("\n")
                if line.startswith("!"):
                    section = line[1:].strip()
                    continue
                m = re.match(r"^\s+(\S+)\s+(.*)$", line)
                if not m:
                    continue
                if section == "layout":
                    names[m.group(1)] = m.group(2).strip()
                elif section == "variant":
                    mv = re.match(r"^([^:]+):\s*(.*)$", m.group(2))
                    if mv:
                        variants.setdefault(mv.group(1).strip(), []).append((m.group(1), mv.group(2).strip()))
    except OSError:
        pass
    if not names:
        names = {"us": "English (US)", "ru": "Russian", "de": "German", "fr": "French", "es": "Spanish"}
    return sorted(((c, n, variants.get(c, [])) for c, n in names.items()), key=lambda x: x[1].lower())


def installed_version(prefix):
    """The version of an existing installation (from the marker the installer writes), or None."""
    try:
        with open(os.path.join(prefix, ".infinity-hyprland"), encoding="utf-8") as f:
            for line in f:
                if line.startswith("version="):
                    return line.split("=", 1)[1].strip() or "unknown"
    except OSError:
        pass
    # an older installer wrote no marker, but it left its session wrapper
    if os.access(os.path.join(prefix, "bin", "infinity-hyprland-session"), os.X_OK):
        return "an older version"
    return None


class State:
    """Everything the user chooses."""

    def __init__(self):
        self.prefix = "/opt/infinity-hyprland"
        self.install_deps = True
        self.jobs = "auto"
        self.has_own = os.path.isfile(os.path.join(os.environ.get("XDG_CONFIG_HOME") or os.path.expanduser("~/.config"), "infinity-hyprland", "hyprland.lua"))
        self.config = "skip" if self.has_own else ("copy" if os.path.isfile(os.path.expanduser("~/.config/hypr/hyprland.lua")) else "fresh")
        self.layouts = []            # [(code, variant, name)]
        self.kb_mode = "system"
        self.kb_default = -1
        self.caps = "normal"
        self.theme = "dark"

    def args(self):
        a = ["--plain", "--yes", "--config", self.config, "--prefix", self.prefix, "--theme", self.theme,
             "--kb-mode", self.kb_mode, "--kb-default", str(self.kb_default), "--caps", self.caps]
        if self.jobs != "auto":
            a += ["--jobs", str(self.jobs)]
        if not self.install_deps:
            a.append("--no-deps")
        if self.layouts:
            a += ["--layouts", ",".join(c + (":" + v if v else "") for c, v, _ in self.layouts)]
        return a


# ------------------------------------------------------------------------------------------------ canvas

NORMAL, TITLE, SELECTED, DIM, ACCENT, ERROR, BUTTON, FOCUS = range(8)


class Canvas:
    """What the UI draws on. Curses implements it; tests use TextCanvas."""

    def size(self):
        raise NotImplementedError

    def put(self, y, x, text, style=NORMAL):
        raise NotImplementedError

    def clear(self):
        raise NotImplementedError


class TextCanvas(Canvas):
    def __init__(self, h=24, w=80):
        self.h, self.w = h, w
        self.clear()

    def size(self):
        return self.h, self.w

    def clear(self):
        self.rows = [[" "] * self.w for _ in range(self.h)]

    def put(self, y, x, text, style=NORMAL):
        if not 0 <= y < self.h:
            return
        for i, ch in enumerate(text):
            if 0 <= x + i < self.w:
                self.rows[y][x + i] = ch

    def text(self):
        return "\n".join("".join(r).rstrip() for r in self.rows)


class CursesCanvas(Canvas):
    def __init__(self, scr):
        self.scr = scr
        self.styles = {NORMAL: curses.A_NORMAL, TITLE: curses.A_BOLD, SELECTED: curses.A_REVERSE, DIM: curses.A_DIM,
                       ACCENT: curses.A_BOLD, ERROR: curses.A_BOLD, BUTTON: curses.A_REVERSE, FOCUS: curses.A_REVERSE | curses.A_BOLD}
        if curses.has_colors():
            curses.start_color()
            try:
                curses.use_default_colors()
                bg = -1
            except curses.error:
                bg = curses.COLOR_BLACK
            pairs = {TITLE: (curses.COLOR_CYAN, bg), ACCENT: (curses.COLOR_CYAN, bg), ERROR: (curses.COLOR_RED, bg),
                     SELECTED: (curses.COLOR_BLACK, curses.COLOR_CYAN), BUTTON: (curses.COLOR_BLACK, curses.COLOR_WHITE),
                     FOCUS: (curses.COLOR_BLACK, curses.COLOR_CYAN)}
            for i, (style, (fg, b)) in enumerate(pairs.items(), start=1):
                curses.init_pair(i, fg, b)
                self.styles[style] = curses.color_pair(i) | (curses.A_BOLD if style in (TITLE, ACCENT, FOCUS) else 0)

    def size(self):
        return self.scr.getmaxyx()

    def clear(self):
        self.scr.erase()

    def put(self, y, x, text, style=NORMAL):
        h, w = self.scr.getmaxyx()
        if not 0 <= y < h or x >= w:
            return
        try:
            self.scr.addstr(y, max(0, x), text[: max(0, w - x - (1 if y == h - 1 else 0))], self.styles.get(style, 0))
        except curses.error:
            pass


# ------------------------------------------------------------------------------------------------ widgets

class Item:
    focusable = True

    def __init__(self):
        self.rect = (0, 0, 0, 0)  # y, x, h, w (set when drawn, used by mouse clicks)

    def draw(self, cv, y, x, w, focused):
        raise NotImplementedError

    def key(self, k):
        return False

    def click(self, y, x):
        pass

    @property
    def height(self):
        return 1


class Label(Item):
    focusable = False

    def __init__(self, text, style=NORMAL):
        super().__init__()
        self.text, self.style = text, style

    def draw(self, cv, y, x, w, focused):
        self.rect = (y, x, 1, w)
        cv.put(y, x, self.text, self.style)


class Check(Item):
    def __init__(self, label, get, set_):
        super().__init__()
        self.label, self.get, self.set = label, get, set_

    def toggle(self):
        self.set(not self.get())

    def draw(self, cv, y, x, w, focused):
        self.rect = (y, x, 1, w)
        cv.put(y, x, f"[{'x' if self.get() else ' '}] {self.label}", FOCUS if focused else NORMAL)

    def key(self, k):
        if k in (ord(" "), 10, 13, curses.KEY_ENTER):
            self.toggle()
            return True
        return False

    def click(self, y, x):
        self.toggle()


class Radio(Item):
    def __init__(self, label, get, set_, value, hint=""):
        super().__init__()
        self.label, self.get, self.set, self.value, self.hint = label, get, set_, value, hint

    def draw(self, cv, y, x, w, focused):
        self.rect = (y, x, 1, w)
        cv.put(y, x, f"({'*' if self.get() == self.value else ' '}) {self.label}", FOCUS if focused else NORMAL)
        if self.hint:
            cv.put(y, x + len(self.label) + 6, self.hint, DIM)

    def key(self, k):
        if k in (ord(" "), 10, 13, curses.KEY_ENTER):
            self.set(self.value)
            return True
        return False

    def click(self, y, x):
        self.set(self.value)


class Button(Item):
    def __init__(self, label, action):
        super().__init__()
        self.label, self.action = label, action

    def draw(self, cv, y, x, w, focused):
        text = f" {self.label} "
        self.rect = (y, x, 1, len(text))
        cv.put(y, x, text, FOCUS if focused else BUTTON)

    def key(self, k):
        if k in (ord(" "), 10, 13, curses.KEY_ENTER):
            self.action()
            return True
        return False

    def click(self, y, x):
        self.action()


class Input(Item):
    def __init__(self, label, get, set_):
        super().__init__()
        self.label, self.get, self.set = label, get, set_

    def draw(self, cv, y, x, w, focused):
        self.rect = (y, x, 1, w)
        cv.put(y, x, self.label, NORMAL)
        field = self.get() + ("_" if focused else "")
        cv.put(y, x + len(self.label) + 1, field.ljust(max(8, w - len(self.label) - 2))[: w - len(self.label) - 2], FOCUS if focused else BUTTON)

    def key(self, k):
        v = self.get()
        if k in (curses.KEY_BACKSPACE, 127, 8):
            self.set(v[:-1])
            return True
        if 32 <= k < 127:
            self.set(v + chr(k))
            return True
        return False


class ListBox(Item):
    """A scrollable list; typing filters it. rows(): [(key, text)]"""

    def __init__(self, rows, on_pick, visible=8, filterable=True):
        super().__init__()
        self.rows, self.on_pick, self.visible, self.filterable = rows, on_pick, visible, filterable
        self.sel, self.top, self.filter = 0, 0, ""

    @property
    def height(self):
        return self.visible + (1 if self.filterable else 0)

    def shown(self):
        f = self.filter.lower()
        return [r for r in self.rows() if not f or f in r[1].lower()]

    def clamp(self, n):
        self.sel = max(0, min(self.sel, max(0, n - 1)))
        if self.sel < self.top:
            self.top = self.sel
        if self.sel >= self.top + self.visible:
            self.top = self.sel - self.visible + 1

    def draw(self, cv, y, x, w, focused):
        self.rect = (y, x, self.height, w)
        rows = self.shown()
        self.clamp(len(rows))
        base = y
        if self.filterable:
            cv.put(y, x, ("Search: " + self.filter + ("_" if focused else "")).ljust(w)[:w], DIM if not focused else NORMAL)
            base += 1
        for i in range(self.visible):
            idx = self.top + i
            if idx >= len(rows):
                cv.put(base + i, x, " " * w)
                continue
            line = ("> " if idx == self.sel else "  ") + rows[idx][1]
            cv.put(base + i, x, line.ljust(w)[:w], (FOCUS if focused else SELECTED) if idx == self.sel else NORMAL)

    def key(self, k):
        rows = self.shown()
        if k == curses.KEY_DOWN:
            self.sel += 1
        elif k == curses.KEY_UP:
            self.sel -= 1
        elif k == curses.KEY_NPAGE:
            self.sel += self.visible
        elif k == curses.KEY_PPAGE:
            self.sel -= self.visible
        elif k in (10, 13, curses.KEY_ENTER):
            if rows:
                self.clamp(len(rows))
                self.on_pick(rows[self.sel][0])
            return True
        elif self.filterable and k in (curses.KEY_BACKSPACE, 127, 8):
            self.filter = self.filter[:-1]
            self.sel = self.top = 0
        elif self.filterable and 32 <= k < 127:
            self.filter += chr(k)
            self.sel = self.top = 0
        else:
            return False
        self.clamp(len(self.shown()))
        return True

    def click(self, y, x):
        row = y - self.rect[0] - (1 if self.filterable else 0)
        rows = self.shown()
        if 0 <= row < self.visible and self.top + row < len(rows):
            self.sel = self.top + row
            self.on_pick(rows[self.sel][0])

    def scroll(self, d):
        self.sel += d
        self.clamp(len(self.shown()))


# ------------------------------------------------------------------------------------------------ pages

class Page:
    title = ""

    def __init__(self, app):
        self.app, self.s = app, app.state
        self.items = self.build()
        self.focus = next((i for i, it in enumerate(self.items) if it.focusable), 0)

    def build(self):
        return []

    def valid(self):
        return None  # an error text blocks "Next"


class Welcome(Page):
    title = "Infinity-hyprland"

    def build(self):
        prev = installed_version(self.s.prefix)
        mode = [Label(f"Found an installation: {prev}. This run will UPDATE it; your config is kept.", ACCENT), Label("")] if prev else \
               [Label("No installation found: this is a fresh install.", ACCENT), Label("")]
        return mode + [Label("A Hyprland fork with an infinite canvas and an Alt-Tab window switcher.", NORMAL), Label(""),
                Label("This installer will:", NORMAL),
                Label("  - install it into its own folder; your Hyprland is not touched", DIM),
                Label("  - add \"Infinity-hyprland\" to the login screen", DIM),
                Label("  - use its own config folder (~/.config/infinity-hyprland)", DIM),
                Label("  - set up your keyboard layouts", DIM), Label(""),
                Label("Keys: arrows / Tab to move, Space or Enter to choose, mouse clicks work too.", DIM),
                Label("Esc or q quits. The build takes 10-30 minutes.", DIM)]


class Options(Page):
    title = "Installation"

    def build(self):
        s = self.s
        has_hypr = os.path.isfile(os.path.expanduser("~/.config/hypr/hyprland.lua"))
        note = [Label("You already have an Infinity-hyprland config: it is kept as it is (old binds are moved to the settings).", DIM)] if s.has_own else []
        return note + [Label("Config to use", ACCENT),
                Radio("Copy my Hyprland config (original stays untouched)", lambda: s.config, lambda v: setattr(s, "config", v), "copy",
                      "" if has_hypr else "(none found)"),
                Radio("Start with a clean config", lambda: s.config, lambda v: setattr(s, "config", v), "fresh"),
                Radio("I will set it up myself", lambda: s.config, lambda v: setattr(s, "config", v), "skip"), Label(""),
                Input("Install into:", lambda: s.prefix, lambda v: setattr(s, "prefix", v)),
                Check("Install missing packages with pacman (needs sudo)", lambda: s.install_deps, lambda v: setattr(s, "install_deps", v)),
                Label(""), Label("Build threads", ACCENT),
                Radio("Automatic (by memory)", lambda: s.jobs, lambda v: setattr(s, "jobs", v), "auto"),
                Radio("2", lambda: s.jobs, lambda v: setattr(s, "jobs", v), 2),
                Radio("4", lambda: s.jobs, lambda v: setattr(s, "jobs", v), 4),
                Radio("6", lambda: s.jobs, lambda v: setattr(s, "jobs", v), 6)]

    def valid(self):
        if not self.s.prefix.startswith("/"):
            return "The install folder must be an absolute path"
        if self.s.config == "copy" and not os.path.isfile(os.path.expanduser("~/.config/hypr/hyprland.lua")):
            return "No ~/.config/hypr/hyprland.lua to copy: pick another option"
        return None


class Languages(Page):
    title = "Keyboard languages"

    def build(self):
        s = self.s
        self.all = read_layouts()
        self.pending = None   # (code, name, variants) waiting for a variant choice
        self.listbox = ListBox(self.rows, self.pick, visible=7)
        self.variant_list = ListBox(self.variant_rows, self.pick_variant, visible=7, filterable=False)
        self.added = ListBox(lambda: [(i, f"{n}  [{c}{'/' + v if v else ''}]") for i, (c, v, n) in enumerate(s.layouts)], self.remove, visible=3, filterable=False)
        return [Label("Pick the languages you type in. Type to search, Enter to add.", DIM), self.listbox, Label(""),
                Label("Added (Enter removes the selected one):", ACCENT), self.added]

    def rows(self):
        return [(c, f"{n}  [{c}]") for c, n, _ in self.all]

    def variant_rows(self):
        return [("", "Default")] + [(v, n) for v, n in (self.pending[2] if self.pending else [])]

    def pick(self, code):
        entry = next(e for e in self.all if e[0] == code)
        if entry[2]:
            self.pending = entry
            self.items = [Label(f"{entry[1]}: choose a variant", ACCENT), self.variant_list]
            self.focus = 1
            self.variant_list.sel = self.variant_list.top = 0
        else:
            self.s.layouts.append((entry[0], "", entry[1]))

    def pick_variant(self, variant):
        self.s.layouts.append((self.pending[0], variant, self.pending[1]))
        self.pending = None
        self.items = [Label("Pick the languages you type in. Type to search, Enter to add.", DIM), self.listbox, Label(""),
                      Label("Added (Enter removes the selected one):", ACCENT), self.added]
        self.focus = 1

    def remove(self, idx):
        if 0 <= idx < len(self.s.layouts):
            del self.s.layouts[idx]


class Typing(Page):
    title = "Typing"

    def build(self):
        s = self.s
        items = [Label("Layout switching (SUPER+SPACE)", ACCENT),
                 Radio("Shared: one layout for everything", lambda: s.kb_mode, lambda v: setattr(s, "kb_mode", v), "system"),
                 Radio("Per window: every window remembers its own layout", lambda: s.kb_mode, lambda v: setattr(s, "kb_mode", v), "per_window"),
                 Label(""), Label("Layout of a new window (per-window mode)", ACCENT),
                 Radio("Keep the current one", lambda: s.kb_default, lambda v: setattr(s, "kb_default", v), -1)]
        for i, (c, v, n) in enumerate(s.layouts):
            items.append(Radio(n, lambda: s.kb_default, lambda v2: setattr(s, "kb_default", v2), i))
        items += [Label(""), Label("Caps Lock", ACCENT),
                  Radio("Normal", lambda: s.caps, lambda v: setattr(s, "caps", v), "normal"),
                  Radio("Per window: on in new windows, remembered for each window", lambda: s.caps, lambda v: setattr(s, "caps", v), "per_window"),
                  Radio("Always on, everywhere", lambda: s.caps, lambda v: setattr(s, "caps", v), "always")]
        return items


class Theme(Page):
    title = "Color scheme"

    def build(self):
        s = self.s
        return [Label("One palette is used for everything: the window switcher, the settings, borders.", DIM),
                Label("You can edit every color later in the settings (SUPER+I).", DIM), Label(""),
                Radio("Dark", lambda: s.theme, lambda v: setattr(s, "theme", v), "dark"),
                Radio("Light", lambda: s.theme, lambda v: setattr(s, "theme", v), "light")]


class Summary(Page):
    title = "Ready to install"

    def build(self):
        s = self.s
        lay = ", ".join(n for _, _, n in s.layouts) or "(your own settings)"
        modes = {"copy": "copy of my Hyprland config", "fresh": "clean config", "skip": "set up by myself"}
        prev = installed_version(s.prefix)
        return [Label(f"This is:        {'an update of ' + prev if prev else 'a fresh install'}"),
                Label(f"Install into:   {s.prefix}"), Label(f"Config:         {modes[s.config]}"),
                Label(f"Languages:      {lay}"),
                Label(f"Layout mode:    {'per window' if s.kb_mode == 'per_window' else 'shared'}"),
                Label(f"Caps Lock:      {s.caps.replace('_', ' ')}"), Label(f"Color scheme:   {s.theme}"),
                Label(f"Threads:        {s.jobs}"), Label(""),
                Label("The installer window closes and the build runs in this terminal (sudo may ask for a password).", DIM)]


# ------------------------------------------------------------------------------------------------ app

class App:
    def __init__(self):
        self.state = State()
        self.classes = [Welcome, Options, Languages, Typing, Theme, Summary]
        self.index = 0
        self.page = self.classes[0](self)
        self.message = ""
        self.result = None      # None, "install" or "quit"
        self.regions = []

    # ---- navigation
    def goto(self, i):
        self.index = max(0, min(len(self.classes) - 1, i))
        self.page = self.classes[self.index](self)
        self.message = ""
        trace = os.environ.get("INFINITY_TUI_TRACE")   # tests: a log of the pages visited
        if trace:
            with open(trace, "a") as f:
                f.write(f"page {self.index} {self.page.title}\n")

    def next(self):
        err = self.page.valid()
        if err:
            self.message = err
            return
        if self.index == len(self.classes) - 1:
            self.result = "install"
        else:
            self.goto(self.index + 1)

    def back(self):
        if self.index > 0:
            self.goto(self.index - 1)

    def buttons(self):
        last = self.index == len(self.classes) - 1
        out = []
        if self.index > 0:
            out.append(Button("Back", self.back))
        out.append(Button("Install" if last else "Next", self.next))
        out.append(Button("Quit", lambda: setattr(self, "result", "quit")))
        return out

    # ---- drawing
    def focusables(self):
        return [it for it in self.page.items if it.focusable] + self.nav

    def draw(self, cv):
        cv.clear()
        h, w = cv.size()
        self.regions = []
        title = f" {self.page.title} "
        cv.put(0, 0, "Infinity-hyprland installer".ljust(w)[:w], FOCUS)
        cv.put(1, 2, title, TITLE)
        cv.put(1, w - 14, f"step {self.index + 1}/{len(self.classes)}", DIM)

        self.nav = self.buttons()
        y = 3
        x = 3
        width = max(20, w - 6)
        for it in self.page.items:
            focused = it.focusable and self.focusables()[self.focus_index()] is it
            it.draw(cv, y, x, width, focused)
            self.regions.append(it)
            y += it.height
        # buttons
        bx = 3
        by = h - 3
        for b in self.nav:
            focused = self.focusables()[self.focus_index()] is b
            b.draw(cv, by, bx, 12, focused)
            self.regions.append(b)
            bx += b.rect[3] + 2
        cv.put(h - 5 if self.message else h - 1, 3, "", NORMAL)
        if self.message:
            cv.put(h - 5, 3, self.message[: w - 6], ERROR)
        cv.put(h - 1, 0, " Tab: next field   Space/Enter: choose   Esc: quit".ljust(w)[:w], DIM)

    def focus_index(self):
        n = len(self.focusables())
        if not hasattr(self, "_focus") or self._focus_page is not self.page:
            self._focus_page = self.page
            self._focus = next((i for i, it in enumerate(self.focusables()) if it.focusable), 0)
        self._focus = max(0, min(self._focus, n - 1))
        return self._focus

    def set_focus(self, it):
        fl = self.focusables()
        if it in fl:
            self._focus_page = self.page
            self._focus = fl.index(it)

    # ---- input
    def key(self, k):
        if k in (27, ord("q")) and not isinstance(self.focusables()[self.focus_index()], (Input, ListBox)):
            self.result = "quit"
            return
        if k == 27:
            self.result = "quit"
            return
        fl = self.focusables()
        cur = fl[self.focus_index()]
        if cur.key(k):
            return
        if k in (9, curses.KEY_DOWN, curses.KEY_RIGHT):
            self._focus = (self.focus_index() + 1) % len(fl)
        elif k in (curses.KEY_BTAB, curses.KEY_UP, curses.KEY_LEFT):
            self._focus = (self.focus_index() - 1) % len(fl)
        elif k == curses.KEY_NPAGE:
            self.next()
        elif k == curses.KEY_PPAGE:
            self.back()

    def click(self, y, x):
        for it in reversed(self.regions):
            ry, rx, rh, rw = it.rect
            if it.focusable and ry <= y < ry + rh and rx <= x < rx + rw:
                self.set_focus(it)
                it.click(y, x)
                return

    def wheel(self, y, x, d):
        for it in self.regions:
            ry, rx, rh, rw = it.rect
            if isinstance(it, ListBox) and ry <= y < ry + rh and rx <= x < rx + rw:
                it.scroll(d)


def run(scr):
    try:
        curses.curs_set(0)
    except curses.error:
        pass
    scr.keypad(True)
    avail, _ = curses.mousemask(curses.ALL_MOUSE_EVENTS | curses.REPORT_MOUSE_POSITION)
    curses.mouseinterval(0)
    cv = CursesCanvas(scr)
    app = App()
    while app.result is None:
        app.draw(cv)
        scr.refresh()
        k = scr.getch()
        if k == curses.KEY_MOUSE:
            try:
                _, mx, my, _, bstate = curses.getmouse()
            except curses.error:
                continue
            up = getattr(curses, "BUTTON4_PRESSED", 0)
            down = getattr(curses, "BUTTON5_PRESSED", 0)
            if bstate & up:
                app.wheel(my, mx, -1)
            elif down and bstate & down:
                app.wheel(my, mx, 1)
            elif bstate & (curses.BUTTON1_PRESSED | curses.BUTTON1_CLICKED):
                app.click(my, mx)
        elif k == curses.KEY_RESIZE:
            continue
        elif k != -1:
            app.key(k)
    return app


def main():
    locale.setlocale(locale.LC_ALL, "")
    os.environ.setdefault("ESCDELAY", "50")   # Esc should not wait a second
    if "--selftest" in sys.argv:
        return selftest()
    app = curses.wrapper(run)
    if app.result != "install":
        print("Cancelled, nothing was changed.")
        return 1

    cmd = [os.path.join(ROOT, "install.sh")] + app.state.args()
    print("\n$", " ".join(cmd), "\n")
    rc = subprocess.call(cmd)
    print("\nDone." if rc == 0 else f"\nThe installer failed (exit code {rc}).")
    return rc


def selftest():
    """Headless checks of the logic: events are fed programmatically."""
    app = App()
    cv = TextCanvas()

    def press(*keys):
        for k in keys:
            app.key(ord(k) if isinstance(k, str) else k)
        app.draw(cv)

    app.draw(cv)
    assert "Infinity-hyprland installer" in cv.text()
    # Welcome -> Options (click the Next button)
    nxt = next(b for b in app.nav if b.label == "Next")
    app.click(nxt.rect[0], nxt.rect[1] + 1)
    app.draw(cv)
    assert app.page.title == "Installation", app.page.title
    assert "Copy my Hyprland config" in cv.text()
    # click "clean config" radio
    radio = next(it for it in app.page.items if isinstance(it, Radio) and it.value == "fresh")
    app.click(radio.rect[0], radio.rect[1] + 1)
    assert app.state.config == "fresh"
    # next -> Languages
    app.draw(cv)
    app.click(*[(b.rect[0], b.rect[1] + 1) for b in app.nav if b.label == "Next"][0])
    app.draw(cv)
    assert app.page.title == "Keyboard languages"
    lb = app.page.listbox
    # type to filter, add Russian (it has variants -> variant list), pick the default one
    app.focus = 0
    app._focus = app.focusables().index(lb)
    for ch in "russian":
        app.key(ord(ch))
    app.draw(cv)
    assert any("Russian" in r[1] for r in lb.shown()), lb.shown()
    app.key(10)
    app.draw(cv)
    assert "choose a variant" in cv.text()
    app.key(10)               # the "Default" variant
    app.draw(cv)
    assert app.state.layouts and app.state.layouts[-1][0] == "ru", app.state.layouts
    # add English too
    lb = app.page.listbox
    app._focus = app.focusables().index(lb)
    lb.filter = ""
    for ch in "english (us)":
        app.key(ord(ch))
    app.key(10)
    app.draw(cv)
    if app.page.pending:       # English (US) has variants
        app.key(10)
    codes = [c for c, _, _ in app.state.layouts]
    assert codes[-2:] == ["ru", "us"], codes
    # Typing page: per-window mode, caps per window
    app.next()
    app.draw(cv)
    assert app.page.title == "Typing"
    app.click(*[(it.rect[0], it.rect[1]) for it in app.page.items if isinstance(it, Radio) and it.value == "per_window"][0:1][0])
    cap = [it for it in app.page.items if isinstance(it, Radio) and it.value == "per_window"][1]
    app.click(cap.rect[0], cap.rect[1])
    assert app.state.kb_mode == "per_window" and app.state.caps == "per_window", (app.state.kb_mode, app.state.caps)
    # theme + summary
    app.next(); app.next()
    app.draw(cv)
    assert app.page.title == "Ready to install"
    args = app.state.args()
    assert "--layouts" in args and args[args.index("--layouts") + 1] == "ru,us", args
    assert args[args.index("--kb-mode") + 1] == "per_window" and args[args.index("--caps") + 1] == "per_window"
    assert "--plain" in args and "--yes" in args
    # validation: a relative install folder blocks Next
    app.goto(1)
    app.state.prefix = "relative"
    app.next()
    assert app.index == 1 and app.message, "an invalid install folder must block Next"
    print("selftest ok")
    return 0


if __name__ == "__main__":
    sys.exit(main())
