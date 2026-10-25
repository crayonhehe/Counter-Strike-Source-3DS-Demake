# Dust II 2DS — gameplay prototype v2

This is a homebrew prototype for Nintendo 2DS/3DS using devkitPro, libctru, citro2d and citro3d. It remains a lightweight pseudo-3D prototype, not a finished full 3D Counter-Strike port.

## What v2 adds

- Multiple named Terrorist teammates: `poo ringo`, `poo shark`, `poo zach`.
- Four opposing CT bots with basic approach-and-attack behaviour.
- Basic health, armour, ammo, reload, kills and money reward tracking.
- A touch-operated buy menu with Glock, AK-47, AWP and armour options. AK-47/AWP/armour require enough money; round-start buy window is brief.
- Two schematic bombsite locations, a plant action near a site, bomb countdown and basic round win/loss/restart flow.
- Bot markers on the lower-screen tactical map.
- GitHub Actions attempts to build both `.3dsx` and `.cia`; CIA output depends on `makerom` and `bannertool` being available in the runner.

## Controls

- Circle Pad: move
- Touchscreen drag: look around
- L: aim; AWP displays a simple scope overlay
- R: fire
- A: reload
- Y: cycle weapons
- X: open/close buy menu (tap a row on the bottom screen)
- START: plant when near a schematic bombsite; START again after a round ends to restart

## Build with GitHub Actions

Push the project files to the repository's `main` or `master` branch, then open **Actions**. A successful run uploads `dust2ds-3dsx-v2`. If CIA tools are present and packaging succeeds, it also uploads `dust2ds-cia-v2`.

## Important limitations

This version is an incremental gameplay prototype. It still uses drawn 2D shapes for a pseudo-3D view; it does not contain a true 3D Dust II level, wall collision, pathfinding/navmesh, proper team tactics, weapon-specific recoil/ballistics, accurate Counter-Strike economy, defuse mechanics, multiplayer, original Counter-Strike assets, or real hardware performance validation. Bot targeting and round logic are deliberately simplified. The CIA workflow has not yet been verified on your repository; a green build and a CIA artifact are required before we can claim it works.
