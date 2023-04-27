#include "LedControl.h" // Import the LedControl library for controlling the LED matrix display
#include "Timer.h" // Import the Timer library
#include <Keypad.h> // Import the Keypad library
#include <LiquidCrystal_I2C.h> // Import the LiquidCrystal_I2C library
 
#define POTPIN A5 // Define the pin for the potentiometer
#define PADSIZE 3 // Define the size of the pad
#define GAME_DELAY 10 // Define the game delay in milliseconds
#define BOUNCE_VERTICAL 1 // Define the vertical bounce direction
#define BOUNCE_HORIZONTAL -1 // Define the horizontal bounce direction
#define NEW_GAME_ANIMATION_SPEED 50 // Define the speed of the new game animation
#define HIT_NONE 0 // Define the hit status as none
#define HIT_CENTER 1 // Define the hit status as center
#define HIT_LEFT 2 // Define the hit status as left
#define HIT_RIGHT 3 // Define the hit status as right
//#define DEBUG 1 // Commented out line, when uncommented will allow debug output through Serial monitor
 
int buzz = 32; // Define the pin for the buzzer
 
// Create an instance of the LiquidCrystal_I2C class with the I2C address 0x27, 16 columns, and 2 rows
LiquidCrystal_I2C lcd(0x27, 16, 2);
 
// Define the interrupt pin
const byte interruptPin = 2;
 
int BALL_DELAY = 300; // Define the ball delay in milliseconds
int redPin = A10; // Define the red pin
int greenPin = A9; // Define the green pin
int bluePin = A8; // Define the blue pin
int score = 0; // Define the initial score as 0
int high = 0; // Define the initial high score as 0
 
const byte ROWS = 4; // Define the number of rows on the keypad
const byte COLS = 4; // Define the number of columns on the keypad
 
// Define a 4x4 keypad array
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
 
// Define an array of row pins for the keypad
byte rowPins[ROWS] = {26, 27, 28, 29};
// Define an array of column pins for the keypad
byte colPins[COLS] = {25, 24, 23, 22};
// Create an instance of the Keypad class with the keypad array, row pins, column pins, number of rows, and number of columns
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);
 
// Define the byte arrays for the sad and smiley face sprites
byte sad[] = {
  B00000000,
  B01000100,
  B00010000,
  B00010000,
  B00000000,
  B00111000,
  B01000100,
  B00000000
};
byte smile[] = {
  B00000000,
  B01000100,
  B00010000,
  B00010000,
  B00010000,
  B01000100,
  B00111000,
  B00000000
};
 
// Create an instance of the Timer class
Timer timer;
LedControl lc = LedControl(12,11,10,1);   // creates an instance of the LedControl class with parameters for the DIN, CLK, and CS pins, as well as the number of LED matrix displays

byte direction;   // initializes a variable to store the direction of the ball movement, with 0 being north
int xball;   // initializes a variable to store the x-coordinate of the ball
int yball;   // initializes a variable to store the y-coordinate of the ball
int yball_prev;   // initializes a variable to store the previous y-coordinate of the ball
byte xpad;   // initializes a variable to store the x-coordinate of the pad
int ball_timer;   // initializes a variable to store the time since the ball was last moved
 
void setSprite(byte *sprite){   // defines a function that takes a pointer to an array of bytes as an argument
    for(int r = 0; r < 8; r++){   // iterates through the rows of the LED matrix
        lc.setRow(0, r, sprite[r]);   // sets the LEDs in the current row to the values specified in the sprite array
    }
}
 
void newGame() {   // defines a function to initialize a new game
    lc.clearDisplay(0);   // clears the LED matrix
    xball = random(1, 7);   // generates a random x-coordinate for the ball between 1 and 6
    yball = 1;   // sets the y-coordinate of the ball to 1
    direction = random(3, 6);   // generates a random direction for the ball between 3 and 5 (southward)
    for(int r = 0; r < 8; r++){   // iterates through the rows of the LED matrix
        for(int c = 0; c < 8; c++){   // iterates through the columns of the LED matrix
            lc.setLed(0, r, c, HIGH);   // sets the LED at the current row and column to "on"
            delay(NEW_GAME_ANIMATION_SPEED);   // pauses for a short duration to create an animation effect
        }
    }
    setSprite(smile);   // sets the sprite to a smiley face
    delay(1500);   // pauses for a short duration
    lc.clearDisplay(0);   // clears the LED matrix
}
 
void setPad() {   // defines a function to set the position of the pad based on an analog input
    xpad = map(analogRead(POTPIN), 0, 1020, 8 - PADSIZE, 0);   // maps the value of the analog input to a position on the LED matrix
}
 
void debug(const char* desc){   // defines a function to print debug information to the serial monitor
#ifdef DEBUG   // checks if the DEBUG macro is defined
    Serial.print(desc);   // prints the description of the debug information
    Serial.print(" XY: ");   // prints the x- and y-coordinates of the ball
    Serial.print(xball);
    Serial.print(", ");
    Serial.print(yball);
    Serial.print(" XPAD: ");   // prints the x-coordinate of the pad
    Serial.print(xpad);
    Serial.print(" DIR: ");   // prints the direction of the ball movement
    Serial.println(direction);
#endif
}
 
// Function to check if the ball has hit a wall and needs to bounce back
int checkBounce() {
    // If the ball is at any edge of the screen, set the bounce direction
    if(!xball || !yball || xball == 7 || yball == 6){
        // Set the bounce direction based on whether the ball hit a horizontal or vertical wall
        int bounce = (yball == 0 || yball == 6) ? BOUNCE_HORIZONTAL : BOUNCE_VERTICAL;
#ifdef DEBUG
        // Print debug information if DEBUG is defined
        debug(bounce == BOUNCE_HORIZONTAL ? "HORIZONTAL" : "VERTICAL");
#endif
        // Return the bounce direction
        return bounce;
    }
    // If the ball hasn't hit a wall, return 0
    return 0;
}

// Function to check if the ball has hit the paddle and where it hit
int getHit() {
    // If the ball hasn't hit the paddle, return HIT_NONE
    if(yball != 6 || xball < xpad || xball > xpad + PADSIZE){
        return HIT_NONE;
    }
    // If the ball hit the center of the paddle, return HIT_CENTER
    if(xball == xpad + PADSIZE / 2){
        return HIT_CENTER;
    }
    // If the ball hit the left or right side of the paddle, return HIT_LEFT or HIT_RIGHT respectively
    return xball < xpad + PADSIZE / 2 ? HIT_LEFT : HIT_RIGHT;
}

// Function to check if the ball has gone past the paddle and the game is over
bool checkLoose() {
    // If the ball has reached the bottom of the screen and didn't hit the paddle, return true
    return yball == 6 && getHit() == HIT_NONE;
}

// Function to move the ball
void moveBall() {
    debug("MOVE");
    int bounce = checkBounce();
    if(bounce) {
        switch(direction){
            case 0:
                direction = 4;
            break;
            case 1:
                direction = (bounce == BOUNCE_VERTICAL) ? 7 : 3;
            break;
            case 2:
                direction = 6;
            break;
            case 6:
                direction = 2;
            break;
            case 7:
                direction = (bounce == BOUNCE_VERTICAL) ? 1 : 5;
            break;
            case 5:
                direction = (bounce == BOUNCE_VERTICAL) ? 3 : 7;
            break;
            case 3:
                direction = (bounce == BOUNCE_VERTICAL) ? 5 : 1;
            break;
            case 4:
                direction = 0;
            break;
        }
        debug("->");
    }
 
    // Check hit: modify direction is left or right
    switch(getHit()){
        case HIT_LEFT:
            if(direction == 0){
                direction =  7;
            } else if (direction == 1){
                direction = 0;
            }
        break;
        case HIT_RIGHT:
            if(direction == 0){
                direction = 1;
            } else if(direction == 7){
                direction = 0;
            }
        break;
    }
 
    // Check orthogonal directions and borders ...
    if((direction == 0 && xball == 0) || (direction == 4 && xball == 7)){
        direction++;
    }
    if(direction == 0 && xball == 7){
        direction = 7;
    }
    if(direction == 4 && xball == 0){
        direction = 3;
    }
    if(direction == 2 && yball == 0){
        direction = 3;
    }
    if(direction == 2 && yball == 6){
        direction = 1;
    }
    if(direction == 6 && yball == 0){
        direction = 5;
    }
    if(direction == 6 && yball == 6){
        direction = 7;
    }
    
    // "Corner" case
    if(xball == 0 && yball == 0){
        direction = 3;
    }
    if(xball == 0 && yball == 6){
        direction = 1;
    }
    if(xball == 7 && yball == 6){
        direction = 7;
    }
    if(xball == 7 && yball == 0){
        direction = 5;
    }
 
    yball_prev = yball;
    if(2 < direction && direction < 6) {
        yball++;
    } else if(direction != 6 && direction != 2) {
        yball--;
    }
    if(0 < direction && direction < 4) {
        xball++;
    } else if(direction != 0 && direction != 4) {
        xball--;
    }
    xball = max(0, min(7, xball));
    yball = max(0, min(6, yball));
    debug("AFTER MOVE");
}

void gameOver() {
    setSprite(sad); // Set the sprite to a sad face
    delay(1500); // Wait for 1.5 seconds
    lc.clearDisplay(0); // Clear the display of the LED matrix
}
 
void drawGame() {
    if(yball_prev != yball){ // Check if the previous ball position is different from the current one
        lc.setRow(0, yball_prev, 0); // If yes, clear the previous position of the ball on the LED matrix
    }
    lc.setRow(0, yball, byte(1 << (xball))); // Draw the current position of the ball on the LED matrix
    byte padmap = byte(0xFF >> (8 - PADSIZE) << xpad) ; // Create a bit map for the paddle based on its size and position
    
#ifdef DEBUG
    //Serial.println(padmap, BIN); // If the DEBUG macro is defined, print the bit map for the paddle in binary format
#endif
    lc.setRow(0, 7, padmap); // Draw the paddle on the LED matrix
}
 
void setup() {
  Serial.begin(9600); // Initialize the serial communication with a baud rate of 9600
  lcd.begin(); // Initialize the LCD screen
  lcd.backlight(); // Turn on the backlight of the LCD screen
  lcd.home(); // Set the cursor of the LCD screen to the home position
  lcd.print("Hello, world!"); // Display "Hello, world!" on the LCD screen

  pinMode(redPin,OUTPUT) ; // Set the red pin as an output pin
  pinMode(greenPin, OUTPUT); // Set the green pin as an output pin
  pinMode(bluePin, OUTPUT); // Set the blue pin as an output pin
  pinMode(buzz, OUTPUT); // Set the buzzer pin as an output pin

  pinMode(POTPIN, INPUT); // Set the potentiometer pin as an input pin
 
  lc.shutdown(0,false); // Wake up the LED matrix from the shutdown mode
  // Set the brightness to a medium values
  lc.setIntensity(0, 8);
  // and clear the display
  lc.clearDisplay(0); // Clear the display of the LED matrix

  randomSeed(analogRead(0)); // Seed the random number generator with an analog value from pin 0

#ifdef DEBUG
  Serial.begin(9600); // If the DEBUG macro is defined, initialize the serial communication with a baud rate of 9600
  Serial.println("Pong"); // If the DEBUG macro is defined, print "Pong" to the serial monitor
#endif

  pinMode(interruptPin,INPUT_PULLUP); // Set the interrupt pin as an input pin with a pull-up resistor enabled
  attachInterrupt(digitalPinToInterrupt(interruptPin), ouch, RISING); // Attach an interrupt to the interrupt pin with the rising edge trigger and call the "ouch" function
}

void loop() {
    timer.update();
     
    // Move pad
      setPad();
      #ifdef DEBUG
        Serial.println(xpad);
      #endif

    // Update screen
       drawGame();
      
    if(checkLoose()) {
        setcolor(255, 0, 0); // Red Color
        debug("LOOSE");
        score = 0; 
        gameOver();
        newGame();
        setcolor(0, 255, 0); // Green Color
        show_score();
    }
    // check if there's user pressing the keypad
    char key = keypad.getKey();
    // set the BALL_DELAY time to 50, 100, 200, 300 millisec  
      if (key == '1') {
        BALL_DELAY = 50;
        Serial.println("Ball speed = 50%");
        ball_timer = timer.every(BALL_DELAY, moveBall);
      }
      else if (key == '2') {
        BALL_DELAY = 100;
        Serial.println("Ball speed = 100%");
        ball_timer = timer.every(BALL_DELAY, moveBall);
      } 
      else if (key == '3') {
        BALL_DELAY = 200;
        Serial.println("Ball speed = 200%");
        ball_timer = timer.every(BALL_DELAY, moveBall);
      } 
      else if (key == 'A') {
        BALL_DELAY = 300;
        Serial.println("Ball speed = 300%");
        ball_timer = timer.every(BALL_DELAY, moveBall);
      }
    delay(GAME_DELAY);
    // show score to the lcd display 
    show_score();
    Serial.println(getHit());
    // if the pad getting hit, the light will be turned on 
    turn_on_light();
}

// show score function - current score and HIGH score
void show_score() {

  if (getHit() != 0) {
    score++;
  }
  
  if (score > high) {
    high = score;
  }
  
  lcd.clear();
  lcd.home();
  lcd.print("SCORE");
  lcd.setCursor(0, 1);
  lcd.print(score);
  lcd.setCursor(12, 0);
  lcd.print("HIGH");
  lcd.setCursor(12, 1);
  lcd.print(high);
}
// interrupt button- if the user pressing the tact button, the game will be reset  
void ouch(){
  // debouncing the button
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  // If interrupts come faster than 200ms, assume it's a bounce and ignore
  if (interrupt_time - last_interrupt_time > 200) 
  {
    newGame();
    delay(GAME_DELAY);
    Serial.println("interrupt is called");
    score = 0;
    turn_on_light();
    setcolor(236, 220, 71); // Yellow Color
    delay(500) ;
   }
  last_interrupt_time = interrupt_time;
}
// function for setting the RGB led 
void setcolor(int redValue, int greenValue, int blueValue){
analogWrite (redPin, redValue);
analogWrite(greenPin, greenValue);
analogWrite(bluePin, blueValue);
}
// If the ball hit the pad, the light will emit the white light and beep-beep sound 
void turn_on_light(){
  if (getHit() == 1 | getHit() == 2 | getHit() == 3) {
    setcolor(255, 255, 255); // White Color
    digitalWrite(buzz, HIGH);
    delay(50) ;
    setcolor(0, 0, 0); // No Color
    digitalWrite(buzz ,LOW);
    delay(50) ; 
  }
}
