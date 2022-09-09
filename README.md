# Arduino LED Display + Tetris
Tetris clone created for custom-built Arduino 8x8 LED display.

### Gameplay Demo

### Features
- Main tetris mechanics
  - Row elimination
  - Block movement (move w/ joystick, quick drop by holding down, rotate by clicking joystick)
  - Grace period before block lock-in
- Game over animation
- Infinite tetris
  - New game immediately starts after previous game ends
  - Makes for a cool retro decoration on a desk
  
### Code Overview
- Information about the current state of the game stored in global variables
  - Current block stored as an array of pieces (piece is a struct with x-y coords)
  - Block position/other block info
  - Layout of locked in blocks (savedPattern)
  - State of joystick
- In the main loop, I call functions readJoystick() and checkCollisions() to update these global variables. I also call checkGameEnd() and checkRowsFull() to check if the game should be over or if any rows should be eliminated.
