# Updating to new Hyprland versions

[English](updating.md) | [Русский](../ru/updating.md)

The base of the fork is recorded in [UPSTREAM](../../UPSTREAM) (tag and commit of the original). Current base: **Hyprland v0.57.0**.

## How it works

- Branch `upstream` holds **only pristine imports** of the original, one commit per version. Our code never lands there.
- `scripts/sync-upstream.sh [tag]` makes a new import and merges it into a branch `sync/<tag>` created from `main`. The merge base is the previous import, so only files that we changed ourselves can conflict.
- Subproject pins (`scripts/subprojects.pins`) are updated from the original's submodule links; a subproject the original dropped is removed from the pins.
- `main` is never changed by the script. The result is built, tested by hand (`dev/run-nested.sh`) and merged through a pull request.
- `.github/workflows/upstream-sync.yml` checks for a new stable release once a day: a clean merge gives a branch, a test build on Arch and a pull request; conflicts open an issue with the list of files. For automatic pull requests the repository setting "Allow GitHub Actions to create and approve pull requests" must be on.
- Libraries (hyprutils, aquamarine, hyprlang, ...) come from the system. A new Hyprland can need newer versions or new libraries: compare `installer/arch-deps.txt` with the `hyprland` package on every update.

## Manual update

```sh
scripts/sync-upstream.sh v0.58.0      # creates sync/v0.58.0; stops on conflicts
git status                            # conflicts
# resolve, git add, git commit
cmake --build build                   # fix API changes the compiler reports
dev/run-nested.sh                     # test Infinity, zoom, the switcher
git switch main && git merge --ff-only sync/v0.58.0
```

Then make a release (below).

## What an update costs

The 0.56.2 to 0.57.0 update is the reference: 718 files changed upstream, 12 files conflicted (input and keybinds moved to `keybinds/`, windows split into `desktop/view/window/*`, rendering moved to `Render::CRenderContext`, `space()` accessor on workspaces, library changes). Our code in the new files never conflicts; the touch points are listed in [architecture.md](architecture.md). Keep hooks small to keep updates cheap.

## Releases

Version scheme: `v<Hyprland version>-infinity.<n>`, for example `v0.57.0-infinity.1`. The number grows with our own changes on the same Hyprland base and restarts at 1 on a new base.

Every release has release notes with two parts: **what was added** (from [CHANGELOG.md](../../CHANGELOG.md)) and **the source versions used** (Hyprland base, libraries, subprojects). The second part is generated:

```sh
scripts/release-notes.sh              # prints the versions table in Markdown
```

Procedure:

1. Update `CHANGELOG.md` and `CHANGELOG.ru.md` (the new section on top).
2. `git tag -a v0.57.0-infinity.2 -m "..."` on `main`, `git push origin main v0.57.0-infinity.2`.
3. `gh release create v0.57.0-infinity.2 --title "..." --notes-file <notes>`, where the notes are the changelog section plus the output of `scripts/release-notes.sh`.
