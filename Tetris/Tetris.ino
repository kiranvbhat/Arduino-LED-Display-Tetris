/* LED array tetris code
 *
 * This recreation of the retro arcade game tetris takes in joystick input
 * and displays the game on an 8x8 LED array. Game has the main features of the 
 * original tetris: 7 main block types, block rotation, row elimination, game 
 * ends when top row reached, and others.
 * 
 * == Setting up the Serial Monitor ==
 * The Serial Monitor must be configured (bottom-right corner of the screen) as:
 *   - Newline (for the line ending)
 *   - Baud rate 115200
 *
 * ENGR 40M
 * March 2022
 */

// Game and display constants
#define DROP_DELAY 1300
#define JOYSTICK_DELAY 200
#define DELAY 50  // longer delay --> LEDs blink less frequently + game runs slower
#define ELIMINATE_ROW_DELAY 300
#define ENDGAME_DELAY 3000
#define GRACE_PERIOD 1000
#define CLOCKWISE 1  // change to 0 for counter-clockwise rotation

#define BLOCK_BRIGHTNESS 10
#define LOCKED_BRIGHTNESS 1
#define HEIGHT 8
#define WIDTH 8

// Analog joystick values
#define UP 100
#define DOWN 1000
#define RIGHT 850
#define LEFT 200
unsigned long prevJoystickTime = millis();  // previous time that joystick moved a block
bool joystick_clicked = false;  // true when joystick is pressed, prevents debouncing

bool joystick_right = false;
bool joystick_left = false;
bool joystick_down = false;
bool joystick_up = false;

unsigned long prevDropTime = millis();  // previous time that joystick moved a block

// Joystick pin numbers
const byte VRX = A4;
const byte VRY = A5;
const byte SW = 1;

// Display pin numbers
const byte ANODE_PINS[HEIGHT] = {2, 3, 4, 5, A0, A1, A2, A3};
const byte CATHODE_PINS[WIDTH] = {13, 12, 11, 10, 9, 8, 7, 6};

// Different types of blocks (aka Tetriminos)
const char BLOCKS[7] = {'I', 'J', 'L', 'O', 'S', 'T', 'Z'};


// 4 pieces in a block, each piece contains (x, y) position of an LED
struct piece {
  int x;
  int y;
};

// Global game variables
int blockX;  // x position of pivot (center) of current block
int blockY;  // y position of pivot (center) of current block
piece pieces[4];  // array of the 4 pieces in the current block
char blockType = 'Q'; // to start at random block, then remember previous block
bool inGracePeriod;
unsigned long gracePeriodStart;

bool blockAtRightEdge;
bool blockAtLeftEdge;
bool blockAtTopEdge;
bool blockAtBottomEdge;

bool blockOnRight;
bool blockOnLeft;
bool blockOnBottom;
bool blockOnTop;

bool gameOver;

byte savedPattern[WIDTH][HEIGHT];


/* Function: initializeBlock()
 * -----------------
 * Generates a new block at the top middle. Block type is pseudo-randomly generated.
 * Sets all global block variables to their default state.
 */
void initializeBlock() {
  blockX = 3;  // about x-center of board
  blockY = -1;  // directly above the top of the board

  // random(0, 7) picks the same block every time (idk why), so need to pick random from larger range
  int randomNumber = random(0, 10000) % 10;
  
  //while randomNumber is invalid, or results in the same block type as the previous block
  while (randomNumber > 6 || BLOCKS[randomNumber] == blockType) {
    randomNumber = random(0, 10000) % 10;
  }
  blockType = BLOCKS[randomNumber];
  
  if (blockType == 'I') {
    pieces[0] = {blockX, blockY};
    pieces[1] = {blockX - 1, blockY};
    pieces[2] = {blockX + 1, blockY};
    pieces[3] = {blockX + 2, blockY};
  }
  if (blockType == 'J') {
    pieces[0] = {blockX, blockY};
    pieces[1] = {blockX - 1, blockY};
    pieces[2] = {blockX - 1, blockY - 1};
    pieces[3] = {blockX + 1, blockY};
  }
  if (blockType == 'L') {
    pieces[0] = {blockX, blockY};
    pieces[1] = {blockX - 1, blockY};
    pieces[2] = {blockX + 1, blockY};
    pieces[3] = {blockX + 1, blockY - 1};
  }
  if (blockType == 'O') {
    pieces[0] = {blockX, blockY};
    pieces[1] = {blockX, blockY - 1};
    pieces[2] = {blockX + 1, blockY};
    pieces[3] = {blockX + 1, blockY - 1};
  }
  if (blockType == 'S') {
    pieces[0] = {blockX, blockY};
    pieces[1] = {blockX - 1, blockY};
    pieces[2] = {blockX, blockY - 1};
    pieces[3] = {blockX + 1, blockY - 1};
  }
  if (blockType == 'T') {
    pieces[0] = {blockX, blockY};
    pieces[1] = {blockX - 1, blockY};
    pieces[3] = {blockX, blockY - 1};
    pieces[2] = {blockX + 1, blockY};
  }
  if (blockType == 'Z') {
    pieces[0] = {blockX, blockY};
    pieces[1] = {blockX - 1, blockY - 1};
    pieces[3] = {blockX, blockY - 1};
    pieces[2] = {blockX + 1, blockY};
  }

  inGracePeriod = false;
  blockAtRightEdge = false;
  blockAtLeftEdge = false;
  blockAtTopEdge = false;
  blockAtBottomEdge = false;
  blockOnRight = false;
  blockOnLeft = false;
  blockOnTop = false;
  blockOnBottom = false;
}


/* Function: setup()
 * -----------------
 * Sets the game and display to their initial states.
 */
void setup() {
  // set up global game variables
  initializeBlock();
  gameOver = false;
  
  // empty environment to start with
  for (int x = 0; x < WIDTH; x++) {
    for (int y = 0; y < HEIGHT; y++) {
      savedPattern[x][y] = 0;
    }
  }

  // set up joystick
  pinMode(VRX, INPUT);
  pinMode(VRY, INPUT);
  pinMode(SW, INPUT);
  

  // set up display
  for (byte i = 0; i < WIDTH; i++) {
    pinMode(ANODE_PINS[i], OUTPUT);
    pinMode(CATHODE_PINS[i], OUTPUT);

    // Turn all LEDs off (LED turns on when both ANODE and CATHODE are LOW)
    digitalWrite(ANODE_PINS[i], HIGH);
    digitalWrite(CATHODE_PINS[i], HIGH);
  }
  
  Serial.begin(115200);
  Serial.setTimeout(100);
}


/* Function: rotationCollision(piece tempPieces[4], int xOffset, int yOffset)
 * -----------------
 * Takes in the temporary rotated block created in rotate(), and checks if that
 * rotation caused any collisions. Also takes in an x offset and y offset to check
 * if moving the temporary block would allow for rotation. 
 * 
 * Returns true if temp block does not collide with other blocks / go out of bounds.
 */
bool rotationCollision(piece tempPieces[4], int xOffset, int yOffset) {
  // check if temporary piece has conflicts with saved pattern
  bool rotationCollision = false;
  for (int piece = 0; piece < 4; piece++) {
    int x = tempPieces[piece].x + xOffset;
    int y = tempPieces[piece].y + yOffset;
    if (x < 0 || x >= WIDTH || y >= HEIGHT) {
      rotationCollision = true;
    } else if (savedPattern[x][y] == LOCKED_BRIGHTNESS) {
      rotationCollision = true;
    } 
  }
  return rotationCollision;
}


/* Function: rotate()
 * -----------------
 * Performs a rotation on the current block if possible.
 * Preferred pivot is (blockX, blockY), but block will be
 * offset if necessary to perform rotation.
 */
void rotate() {
  if (blockType == 'O') {
    return;
  }

  // make temporary clone of current block, but centered at (0, 0)
  int xDistFromZero = blockX;
  int yDistFromZero = blockY;
  piece tempPieces[4];
  for (int piece = 0; piece < 4; piece++) {
    tempPieces[piece].x = pieces[piece].x - xDistFromZero;
    tempPieces[piece].y = pieces[piece].y - yDistFromZero;
  }

  // Rotate temporary block. 
  // Since pivot is 0, 0: (x, y) --> (-y, x) to rotate clockwise.
  for (int piece = 0; piece < 4; piece++) {
    int x = tempPieces[piece].x;
    int y = tempPieces[piece].y;
    if (CLOCKWISE) {
      tempPieces[piece].x = -y;
      tempPieces[piece].y = x;
    } else {
      tempPieces[piece].x = y;
      tempPieces[piece].y = -x;
    }
    
  }

  // move temporary block to current blocks location
  for (int piece = 0; piece < 4; piece++) {
    tempPieces[piece].x += xDistFromZero;
    tempPieces[piece].y += yDistFromZero;
  }

  // now we'll check if the temporary block collides with anything
  bool collision = true;
  int xOffset;
  int yOffset;

  // check rotation in place
  if (!rotationCollision(tempPieces, 0, 0)) {
    collision = false;
    xOffset = 0;
    yOffset = 0;
  } 

  // check rotation with slight offset
  if (collision) {
    piece offsets[6] = {{-1, 0}, {1, 0}, {0, 1}, {-2, 0}, {2, 0}, {0, 2}}; //checks offsets in order
    for (int i = 0; i < 6; i++) {
      if (!rotationCollision(tempPieces, offsets[i].x, offsets[i].y)) {
        collision = false;
        xOffset = offsets[i].x;
        yOffset = offsets[i].y;
        break;
      }
    }
  }

  // if there wasn't a collision in some case, perform the offset and rotation
  // by setting the the current block to our temp block with offset
  if (!collision) {
    blockX += xOffset;
    blockY += yOffset;
    for (int piece = 0; piece < 4; piece++) {
      pieces[piece].x = tempPieces[piece].x + xOffset;
      pieces[piece].y = tempPieces[piece].y + yOffset;
    }
  }
}


/* Function: eliminateRow(int row)
 * -----------------
 * Eliminates the given row (makes it empty), displays the empty row,
 * and moves blocks down to fill the empty row
 */
void eliminateRow(int row) {
  // empty out row
  for (int x = 0; x < WIDTH; x++) {
    savedPattern[x][row] = 0;
  }

  if (!gameOver) {
    // display pattern with the empty row
    unsigned long eliminateTime = millis();
    while(millis() - eliminateTime < ELIMINATE_ROW_DELAY) {
    display(savedPattern);
    }
  }


  // move blocks down to fill the empty row
  for (int y = row; y >= 1; y--) {
    for (int x = 0; x < WIDTH; x++) {
      savedPattern[x][y] = savedPattern[x][y - 1];
    }
  }

  // clear top row
  for (int x = 0; x < WIDTH; x++) {
      savedPattern[x][0] = 0;
  }

  
  if (gameOver) {
    // display pattern with filled-in row
    unsigned long eliminateTime = millis();
    while(millis() - eliminateTime < ELIMINATE_ROW_DELAY) {
    display(savedPattern);
    }
  }
}


/* Function: endGame()
 * -----------------
 * Performs ending animation (blocks blinking 3 times and falling down), then
 * resets state of game.
 */
void endGame() {
  gameOver = true;
  // animation to end game
  byte blank[8][8];
  for (int x = 0; x < WIDTH; x++) {
    for (int y = 0; y < HEIGHT; y++) {
      blank[x][y] = 0;
    }
  }
  unsigned long endgameTime = millis();
  while (millis() - endgameTime < ENDGAME_DELAY) {
    unsigned long blinkTime = millis();
    while (millis() - blinkTime < ENDGAME_DELAY / 6) {
      display(blank);
    }
    blinkTime = millis();
    while (millis() - blinkTime < ENDGAME_DELAY / 6) {
      display(savedPattern);
    }
  }
  
  for (int i = 0; i < HEIGHT; i++) {
    eliminateRow(HEIGHT - 1);
  }

  // reset state of game
  setup();
}


/* Function: checkGameEnd()
 * -----------------
 * Checks if game should end (if any blocks reach top row).
 * Ends game if necessary.
 */
void checkGameEnd() {
  // reset game if any blocks reach the top row
  for (int x = 0; x < WIDTH; x++) {
    if (savedPattern[x][0] != 0) {
      endGame();
      break;
    }
  }
}


/* Function: checkRowsFull()
 * -----------------
 * Checks if any rows are full. If any are full, eliminates them.
 */
void checkRowsFull() {
  //check for full rows and eliminate them
  for (int y = 1; y < HEIGHT; y++) {
    bool rowFull = true;
    for (int x = 0; x < WIDTH; x++) {
      if (savedPattern[x][y] == 0) {
        rowFull = false;
      }
    }
    if (rowFull) {
      eliminateRow(y);
      y--;  // need to check the same row again after eliminating/dropping blocks
    }
  }
}


/* Function: updatePattern()
 * -----------------
 * Updates pattern with the saved pattern and current block to be drawn.
 */
void updatePattern(byte pattern[8][8]) {
  for (int x = 0; x < 8; x++) {
    for (int y = 0; y < 8; y++) {
      pattern[x][y] = savedPattern[x][y];
    }
  }
  for (int piece = 0; piece < 4; piece++) {
    if (pieces[piece].y >= 0) {
      pattern[pieces[piece].x][pieces[piece].y] = BLOCK_BRIGHTNESS;
    }
  }
}


/* Function: lockInBlock()
 * -----------------
 * Locks the current block in savedPattern, then initializes a new block.
 */
void lockInBlock() {
    for (int piece = 0; piece < 4; piece++) {
      int x = pieces[piece].x;
      int y = pieces[piece].y;
      if (y >= 0) {
        savedPattern[x][y] = LOCKED_BRIGHTNESS;
      }
    }
    initializeBlock();
}


/* Function: checkCollision()
 * -----------------
 * Checks if current block should be locked in place. Does so if necessary.
 */
void checkCollisions() {
  blockAtRightEdge = false;
  blockAtLeftEdge = false;
  blockAtTopEdge = false;
  blockAtBottomEdge = false;
  blockOnRight = false;
  blockOnLeft = false;
  blockOnTop = false;
  blockOnBottom = false;
  
  for (int piece = 0; piece < 4; piece++) {
    int x = pieces[piece].x;
    int y = pieces[piece].y;
    
    if (x >= WIDTH - 1) {
      blockAtRightEdge = true;
    }
    if (x <= 0) {
      blockAtLeftEdge = true;
    }
    if (y <= 0) {
      blockAtTopEdge = true;
    }
    if (y >= HEIGHT - 1) {
      blockAtBottomEdge = true;
    }
    if (y >= 0) {  // y might be less than 0 (directly after initialized), so don't allow indexing into -y in savedPattern
      if (x < WIDTH - 1 && savedPattern[x + 1][y] == LOCKED_BRIGHTNESS) {
        blockOnRight = true;
      }
      if (x > 0 && savedPattern[x - 1][y] == LOCKED_BRIGHTNESS) {
        blockOnLeft = true;
      }
      if (y > 0 && savedPattern[x][y - 1] == LOCKED_BRIGHTNESS) {
        blockOnTop = true;
      }
      if (y < HEIGHT - 1 && savedPattern[x][y + 1] == LOCKED_BRIGHTNESS) {
        blockOnBottom = true;
      }
    }
    
  }

  // if block reaches bottom, lock it in place
  if (blockAtBottomEdge || blockOnBottom) {
    if (inGracePeriod) {
      inGracePeriod = true;
    } else {
      inGracePeriod = true;
      gracePeriodStart = millis();
    }
    
  } else {
    inGracePeriod = false;
  }

  if (inGracePeriod) {
    if (millis() - gracePeriodStart > GRACE_PERIOD) {
      lockInBlock();
    }
  }
}


/* Function: moveBlock()
 * -----------------
 * Moves block depending on which global joystick variables are updated.
 */
void moveBlock() {
  if (joystick_right) {
    checkCollisions();
    if (!blockAtRightEdge && !blockOnRight) {
      blockX++;
      for (int piece = 0; piece < 4; piece++) {
        pieces[piece].x++;
      }
    }
  }

  else if (joystick_left) {
    checkCollisions();
    if (!blockAtLeftEdge && !blockOnLeft) {
      blockX--;
      for (int piece = 0; piece < 4; piece++) {
        pieces[piece].x--;
      }
    }
  }

  if (joystick_down) { // not an else if because we want block to drop at same time as moving left or right
    checkCollisions();
    if (!blockAtBottomEdge && !blockOnBottom) {
      blockY++;
      for (int piece = 0; piece < 4; piece++) {
        pieces[piece].y++;
      }
    }
  }
}


/* Function: readJoystick()
 * -----------------
 * Read joystick and update global joystick variables. Call moveBlock() and/or rotate() depending
 * on what is read.
 */
void readJoystick() {
  
  // movement delay
  if (millis() - prevJoystickTime >= JOYSTICK_DELAY) {
    prevJoystickTime = millis();
    int x = analogRead(VRX);
    int y = analogRead(VRY);
    
    if (x > RIGHT) {
      joystick_right = true;
      moveBlock();
    } else {
      joystick_right = false;
    }
    
    if (x < LEFT && !joystick_right) {
      joystick_left = true;
      moveBlock();
    } else {
      joystick_left = false;
    }

    if (y > DOWN) {
      joystick_down = true;
      moveBlock();
    } else {
      joystick_down = false;
    }
  }

  // no delay for rotation
  if (digitalRead(SW) != 0) {
    joystick_clicked = false;
  }
  if (digitalRead(SW) == 0 && !joystick_clicked) {
    rotate();
    joystick_clicked = true;  // prevent multiclicks (one press with 2+ clicks)
  }
}


/* Function: display
 * -----------------
 * Runs through one multiplexing cycle of the LEDs, controlling which LEDs are
 * on.
 *
 * During this function, LEDs that should be on will be turned on momentarily,
 * one row at a time. When this function returns, all the LEDs will be off
 * again, so it needs to be called continuously for LEDs to be on.
 * 
 * Takes in 2D array of brightnesses (0-15). Lower brightnesses are displayed with fewer blinks.
 * For example, a brightness of 2 would turn on the LED for 2 out of the 15 iterations.
 */
void display(byte pattern[WIDTH][HEIGHT]) {
  // Loop through anodes
  for (byte y = 0; y < HEIGHT; y++) {
      digitalWrite(ANODE_PINS[y], LOW);  // turn anode on (prepare LEDs to turn on)
  
      // For 15 levels of brightness
      for (byte b = 0; b <= 15; b++) {
  
          // Loop through cathodes
          for (byte x = 0; x < WIDTH; x++) {  
  
              // if brightness at (i,j) is greater than the current brightness
              if(pattern[x][y] > b){
                  digitalWrite(CATHODE_PINS[x], LOW);  // then set/keep cathode low (LED on)
              }
              else {
                  digitalWrite(CATHODE_PINS[x], HIGH);  // else turn cathode high (LED off)
              }
              
          }
          delayMicroseconds(DELAY);
      }
      digitalWrite(ANODE_PINS[y], HIGH);  // turn anode off
  }
}


/* Function: loop()
 * -----------------
 * Main game loop. Repeatedly calls helper functions to check and update
 * the state of the game, as well as display the game.
 */
void loop() {
  // check and update game conditions
  checkGameEnd();
  checkRowsFull();

  // move block down
  if (millis() - prevDropTime > DROP_DELAY) {
    joystick_down = true;
    moveBlock();
    joystick_down = false;
    prevDropTime = millis();
  }

  // read joystick and move block if no collisions
  readJoystick();
  checkCollisions();
  
  // Set pattern to current layout of game
  byte pattern[8][8];
  updatePattern(pattern);
  display(pattern);
  
}
