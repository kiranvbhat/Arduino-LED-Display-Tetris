# Arduino LED Display + Tetris
Tetris clone created for custom-built Arduino 8x8 LED display.

### Gameplay Demo (Click GIF for Youtube Video)
[![Arduino LED Display + Tetris](https://imgur.com/A9AC5Fu.gif)](https://youtu.be/hBOwo1rJkic "Arduino LED Display + Tetris")


### Features
- Main tetris mechanics
  - Block movement (move w/ joystick, quick drop by holding down, rotate by clicking joystick)
  - Full row elimination
  - Grace period before block lock-in
- Custom animations
  - Game over animation
  - Row elimination animation
  - Moving block displayed brighter than locked-in blocks
- Infinite tetris
  - New game immediately starts after previous game ends
  - Makes for a cool retro decoration on a desk
  
### Code Overview
- Display pattern is represented as a 2D array of brightnesses (from 0 - 15). Check `display()` function in `Tetris.ino` for more implementation details.
- Information about the current state of the game stored in global variables
  - Current block stored as an array of pieces (`piece` is a struct with x-y coords)
  - Block position/other block info
  - Layout of locked in blocks (savedPattern)
  - State of joystick
- In the main loop, I call functions `readJoystick()` and `checkCollisions()` to update these global variables. I also call `checkGameEnd()` and `checkRowsFull()` to check if the game should be over or if any rows should be eliminated.
