# Capybara Pacman

A Pac-Man style game made with C++ and SFML, themed around capybaras, crocodiles, leaves, water, and powerups.

## Requirements

- C++17 compiler
- CMake 3.28 or newer
- SFML 3

On Linux, SFML must be available to CMake as `SFML 3` with the Graphics, Window, Audio, and System components.

## Build

```bash
cmake -S . -B build
cmake --build build
```

## Run

Run the game from the project root so it can find `assets/` and `fonte.ttf`:

```bash
./build/pacman
```

If you build directly with `g++`, this also works:

```bash
g++ -std=c++17 pacman.cpp -o pacman $(pkg-config --cflags --libs sfml-all)
./pacman
```

## Controls

- Arrow keys: move
- `P`: pause/resume
- `R`: restart during gameplay
- `M`: return to menu
- `T`: test mode with easier crocodiles

## Gameplay Notes

- Leaves add points.
- Melancia grants a bonus heart and starts a short rush mode.
- During rush mode, crocodiles turn blue and slow down.
- Touching a blue crocodile plays a bite animation, sends it back to base, and awards bonus points.
- The game stores your local high score in `highscore.txt`.

## Project Layout

```text
.
├── assets/          # Game images and sounds
├── fonte.ttf        # Font loaded by the game
├── pacman.cpp       # Main game source
└── CMakeLists.txt   # CMake build configuration
```

## Notes

- The executable files `pacman` and `a.out` are ignored because they are local build outputs.
- Keep `assets/` and `fonte.ttf` committed, because the game loads them at runtime.
