#include <FastLED.h>
#include "RTClib.h"
#include <Wire.h>
#include <SparkFunADXL313.h>

#define LED_PIN 6
#define LED_NUM 64
#define BD_NUM 24
#define PR A0
#define BUTT_1 5
#define BUTT_2 4
#define BUTT_3 3
#define MODE_LIM 3
#define BACKG_NUM 1
#define ACCEL_PORT 0x69
#define TIP_POINT 40000
#define FADE_RATE 2
#define RESET 500
#define B_HIGH 100
#define B_LOW 20
#define BLANK CRGB(0,0,0)


DEFINE_GRADIENT_PALETTE(MAP_WHITEBLUE) {
    0, 0, 0, 255,
    128, 0, 100, 155,
    255, 0, 0, 255
};
CRGBPalette16 WhiteBlue = MAP_WHITEBLUE;

DEFINE_GRADIENT_PALETTE(MAP_WHITEGREEN) {
    0, 0, 250, 5,
    128, 100, 100, 50,
    255, 0, 250, 5,
};
CRGBPalette16 WhiteGreen = MAP_WHITEGREEN;

DEFINE_GRADIENT_PALETTE(MAP_ORGRED) {
    0, 230, 0, 15,
    128, 150, 100, 0,
    255, 230, 0, 15
};
CRGBPalette16 OrangeRed = MAP_ORGRED;

DEFINE_GRADIENT_PALETTE(MAP_YLWWHITE) {
    0, 75, 75, 75,
    128, 150, 100, 0,
    255, 75, 75, 75
};
CRGBPalette16 YellowWhite = MAP_YLWWHITE;

DEFINE_GRADIENT_PALETTE(MAP_FIRE) {
    0, 100, 0, 0,
    33, 75, 25, 0,
    66, 100, 0, 0,
    99, 50, 15, 0,
    132, 75, 15, 0,
    185, 80, 0, 0,
    208, 100, 20, 0,
    255, 70, 10, 0

};
CRGBPalette16 FireBackGround = MAP_FIRE;




/*
* Takes a PhotoResistor to read a returns nice smooth values back.
* You can use setLimits to adjust what values you are reciveing.
*/
class PhotoResistorSmoother{ 
  
    public: // Give it the number of the port, along the high and low limits of the values you want back. 
      PhotoResistorSmoother(uint8_t portNum) {
        port = portNum;
      }

      // returns the smoothed out value of the PhotoResistor.
      int getValue() {
        target = analogRead(port);
        value += floor((target - value) / 8);
        return map(value, 7, 1016, lowRead, highRead);
      }

      void setLimits(int32_t low, int32_t high) { // Resets the limits on the photoresistor reader.
        highRead = high;
        lowRead = low;
      }

    private:
      uint8_t port = 0;
      int32_t target = 0;
      int32_t value = 0;
      int32_t highRead = 1023;
      int32_t lowRead = 0;
};
//
 /**
 * The button class takes in a port number to read.
 * Call the check function in your loop to check if the button was pressed.
 * use the result to decide what to do based on how long the button was held down for.
 */

class Button{

    public:
        Button(uint8_t port) {
            Port = port;
        }
        
        uint32_t check() {
            uint32_t count = 0;
            while(digitalRead(Port) == LOW) {
                count++;
                delay(1);
            }
            return count;
        }

    private:
        uint8_t Port = 0;
};

enum class Action{ ModeChange, TimeAdjust, DoNothing};

/**
* The control board has control over all the external controls on the clock.
* This includes the three buttons along with the potenitometer reader.
* The function goIntoAdjustMode is meant to be inside a loop.
* It keeps an eye out for the main button being held down long enough to get tossed into
* adjust mode.
* In this mode it has two function that return either a 1 or -1 to adjust hours and minutes.
* this depends on how long the adjuster buttons have been held down for.
* It will also give out the potentiometer readings. 
*/
class ControlBoard{
    public:
        ControlBoard(uint8_t pin1, uint8_t pin2, uint8_t pin3, uint8_t pin4) : MainButt(pin1), HourButt(pin2), MinButt(pin3), PR_Reader(pin4) {
          PR_Reader.setLimits(B_LOW, B_HIGH);
        }



        uint8_t getMode() {
            return Mode;
        }
        
        Action buttonCheck() { // Returns true if it's time to go into time adjust mode
            uint32_t count = MainButt.check();
            if (count > 1) {
                if (HalfSecond > count) {Mode++; return Action::ModeChange;}
                if (count > HalfSecond && MultiSecond > count) {Mode--; return Action::ModeChange;}
                if (count > MultiSecond) {adjustMode = !adjustMode;}
                ModeLimitCheck();
            }
            if (adjustMode) {
              return Action::TimeAdjust;
            } else {
              return Action::DoNothing;
            }
        }

        int8_t getHourUpdate() {
            uint32_t count = HourButt.check();
            if (count > 1) {
                if (HalfSecond > count) {return 1;}
                if (count > HalfSecond) {return -1;}
            }
            return 0;
        }

        int8_t getMinuteUpdate() {
            uint32_t count = MinButt.check();
            if (count > 1) {
                if (HalfSecond > count) {return 1;}
                if (count > HalfSecond) {return -1;}
            }
            return 0;
        }

        int8_t getPRReading() {
          return PR_Reader.getValue();
        }
        

    private:
        Button MainButt;
        Button HourButt;
        Button MinButt;
        PhotoResistorSmoother PR_Reader; 
        const uint16_t HalfSecond = 500;
        const uint16_t MultiSecond = 5000;
        int8_t Mode = 0;
        bool adjustMode = false;
        void ModeLimitCheck() {
            if (Mode >= MODE_LIM) {
                Mode = 0;
            } 
            if (0 > Mode) {
                Mode = MODE_LIM - 1;
            }
        }

};

class BackGround{
  public:
      // Doesn't work
      void createBackGround(CRGBPalette16 *palPoint, uint8_t rate) {
        ColorPalette = palPoint;
        Rate = rate;
      }

      void createBackGround(CRGB color) {
        SolidColor = color;
      }

      CRGB getColor() {
        if (Rate > 0) {
          EVERY_N_MILLISECONDS(Rate) {
            Increment++;
            if (Increment % Rate == 0) {
              return ColorFromPalette(*ColorPalette, Increment);
            }
          }
        }
        return SolidColor;
      }

  private:
      uint8_t Increment; 
      uint8_t Rate = 0;
      CRGBPalette16 *ColorPalette;
      CRGB SolidColor = BLANK;
};

/**
 * The bitdot class creates a led that will react to gravity along with chanign binary colors
 * Set the fixed location in the formation that you want the clock itself in 
 * Ask the dot to move and it will freely do so when given readings from an accelerometer.
 * To stop it from moving, stop asking it to and tell it to go back to its original locaton. 
 * 
 */

class BitDots{
    public:
        void begin() { 
            FastLED.addLeds<WS2812B, LED_PIN, GRB>(FLEDS, LED_NUM);
            FastLED.setBrightness(255);
        }

        void SHOW_ALL_DOTS() {
          FastLED.show();
        }

        void CLEAR_ALL_DOTS() {
          FastLED.clear();
        }

        void DISPLAY_BACKGROUND(BackGround &BackG) {
          for (int i = 0; i < 8; ++i) {
            for (int j = 0; j < 8; ++j) {
              if (!Locations[i][j]) {
                displayBackground(GetLineLocation(i, j), BackG);
              }
            }
          }
        }

        void ALL_DOT_BRIGHTNESS(uint8_t bright) {
          FastLED.setBrightness(bright);
        }

        void setFixedLocation(int8_t xSet, int8_t ySet) {
            xFixed = xSet;
            x = xFixed;
            yFixed = ySet;
            y = yFixed;
            Locations[xFixed][yFixed] = true;
        }

        void setColorPalette(CRGBPalette16 *pal) {
            ColorPalette = pal;
        }

        void setToOne() {
            if (IsAZero) {
                Fading = true;
                IsAZero = false;
            }
        }

        void setToZero() {
            if (!IsAZero) {
                Fading = true;
                IsAZero = true;
            }
        }
 
        void moveDot(int16_t xRead, int16_t yRead) {
            updatePulls(xRead, yRead); // Update the dots pull values
            int8_t xTemp = shiftDot(xPull, x); // Creates a theoretical dot in the direction it wants to move. This accounts for the board but not other dots.
            int8_t yTemp = shiftDot(yPull, y); 
            if (!Locations[xTemp][yTemp]) { // This where the dot wants to move against the locations of all the other dots.
            Locations[x][y] = false;  // this dot is no longer at that location
            x = xTemp; // If that location was okay move the dot there.
            y = yTemp;
            Locations[x][y] = true; // This dot is now at this location
          }
        }

        bool isInClockFormation() {
            return x == xFixed && y == yFixed;
        }

        void setToClockPosition() {
            Locations[x][y] = false; // Remove the dot from its current location
            x = xFixed;
            y = yFixed;
            Locations[x][y] = true; // Put the dot locatio back with the clock formation
        }

        uint8_t GetLineLocation(int8_t xTemp, int8_t yTemp) {
            return xTemp + (yTemp * 8);
        }

        uint8_t GetLineLocation() {
            return x + (y * 8);
        }

        void displayDot() {
            fadeIfNeeded();
            FLEDS[GetLineLocation()] = ColorFromPalette(*ColorPalette, ColorIndex);
        }



    private:
        uint8_t BGFader;
        int8_t xFixed;
        int8_t yFixed;
        int8_t x;
        int8_t y;
        int16_t xPull;
        int16_t yPull;
        uint8_t ColorIndex;
        bool Fading = false;
        const int16_t pullLimit = 512;
        inline static bool Locations[8][8];
        CRGBPalette16* ColorPalette;
        inline static CRGB FLEDS[LED_NUM];
        bool IsAZero = true;
        int8_t boundCheck(int8_t temp, int8_t incr) { // Make sure the new location is on the board.
              temp += incr;
              if (temp < 0) {
                return 0;
              } else if (7 < temp) {
                return 7;
              }
              return temp;
            }

            int8_t shiftDot(int16_t &tempPull, int8_t temp) { // Measure a pull value and see if it is enough to move a dot in that direction
              if (tempPull < -pullLimit) {
                tempPull = 0;
                return boundCheck(temp, -1);
              } else if (pullLimit < tempPull) {
                tempPull = 0;
                return boundCheck(temp, 1);
              }
              return temp;
            }

            void updatePulls(int16_t xRead, int16_t yRead) {
              xPull += (xRead) / 4; // Increment how hard the dot is being pulled by the readings
              yPull += (yRead)/ 4;
            }

            void fadeIfNeeded() {
                if (Fading) {
                  ColorIndex += FADE_RATE;
                  if (ColorIndex == 128 || ColorIndex == 0) {
                    Fading = false;
                  }
                }
            }

          void displayBackground(int8_t location, BackGround &BGColor) {
            FLEDS[location] = BGColor.getColor();
            //FLEDS[location] = ColorFromPalette(FireBackGround, location * 10);
          }


};

class ClockDisplay{
    public: 
        virtual void getBitDotPointer(BitDots (&bitPoint)[BD_NUM]) {
            BitDotPointer = bitPoint;
        }
        // This sets the fixed loctions and color paletts for the bitdots.
        virtual void buildClock(BitDots (&bitArr)[BD_NUM])=0;
        
        // Updates who is a zero and who is a one. 
        virtual void updateTime(BitDots (&bitArR)[BD_NUM], DateTime now)=0;

        // Lets you know how many bit dots it will need for the display.
        virtual uint8_t requestNumOfDots()=0;


    private:
        BitDots* BitDotPointer;
        // Takes in a bitdot array along with the digit to turn into binary.
        // The final variable is the index for wich we want to start placing this binary digit along the string of bitdots.
        virtual void setColor(BitDots (&bitArr)[BD_NUM], uint8_t index)=0;
};


class SixteenBitWhiteClock:ClockDisplay{
    public:
        void buildClock(BitDots (&bitArr)[BD_NUM]) {
            int x = 0;
            int y = 0;
            for (int i = 0; i < 16; ++i) {
              if (i < 4) {
                y = 5 - i;
                x = 6;
              } else if (i < 10 && 4 <= i) {
                y = 10 - i;
                x = 3;
              } else if (10 <= i) {
                y = 16 - i;
                x = 1;
              }
              bitArr[i].setFixedLocation(x, y);
              setColor(bitArr, i);
            }
        }     

        void updateTime(BitDots (&bitArr)[BD_NUM], DateTime now) { // This desides what color to display the dot. 
            //set the bits for the hours
            int8_t temp = now.hour();
            if (temp > 12) {
              temp -= 12;
            }
            for (int i = 0; i < 4; ++i) {
              if (temp % 2 == 1) {
                bitArr[i].setToOne();
              } else {
                bitArr[i].setToZero();
              }
              temp /= 2;
            }
            // Set the bits for the minutes
            temp = now.minute();
            for (int i = 4; i < 10; ++i) {
              if (temp % 2 == 1) {
                bitArr[i].setToOne();
              } else {
                bitArr[i].setToZero();
              }
              temp /= 2;
            }
          
            // set the bits for the seconds
            temp = now.second();
            for (int i = 10; i < 16; ++i) {
              if (temp % 2 == 1) {
                bitArr[i].setToOne();
              } else {
                bitArr[i].setToZero();
              }
              temp /= 2;
            }
        }

        uint8_t requestNumOfDots() {
          return 16;
        }

    private:
        void setColor(BitDots (&bitArr)[BD_NUM], uint8_t index) {
            bitArr[index].setColorPalette(&YellowWhite);
        }

};


class SixteenBitMultiColClock:ClockDisplay{
    public:
        void buildClock(BitDots (&bitArr)[BD_NUM]) {
            int x = 0;
            int y = 0;
            for (int i = 0; i < 16; ++i) {
              if (i < 4) {
                y = 5 - i;
                x = 6;
                bitArr[i].setColorPalette(&OrangeRed);
              } else if (i < 10 && 4 <= i) {
                y = 10 - i;
                x = 3;
                bitArr[i].setColorPalette(&WhiteGreen);
              } else if (10 <= i) {
                y = 16 - i;
                x = 1;
                bitArr[i].setColorPalette(&WhiteBlue);
              }
              bitArr[i].setFixedLocation(x, y);
              setColor(bitArr, i);
            }
        }     

        void updateTime(BitDots (&bitArr)[BD_NUM], DateTime now) { // This desides what color to display the dot. 
            //set the bits for the hours
            int8_t temp = now.hour();
            if (temp > 12) {
              temp -= 12;
            }
            for (int i = 0; i < 4; ++i) {
              if (temp % 2 == 1) {
                bitArr[i].setToOne();
              } else {
                bitArr[i].setToZero();
              }
              temp /= 2;
            }
            // Set the bits for the minutes
            temp = now.minute();
            for (int i = 4; i < 10; ++i) {
              if (temp % 2 == 1) {
                bitArr[i].setToOne();
              } else {
                bitArr[i].setToZero();
              }
              temp /= 2;
            }
          
            // set the bits for the seconds
            temp = now.second();
            for (int i = 10; i < 16; ++i) {
              if (temp % 2 == 1) {
                bitArr[i].setToOne();
              } else {
                bitArr[i].setToZero();
              }
              temp /= 2;
            }
        }

        uint8_t requestNumOfDots() {
          return 16;
        }

    private:
        void setColor(BitDots (&bitArr)[BD_NUM], uint8_t index) {
        }

};

class CompleteClock{
    public:
        CompleteClock(
            uint8_t pin1,
            uint8_t pin2, 
            uint8_t pin3,
            uint8_t pin4
            ) :
            Controller(pin1, pin2, pin3, pin4) {}

        void begin() {
          Accelerometer.begin();
          BitDotArr[0].begin();
          RTC.begin();
          Clock16Bit.buildClock(BitDotArr);
          createBackGrounds();
        }

        void runClock() {
          readInputs();
          manageController();
          manageGravityMode(); 
          manageDisplayMode();
          for (int i = 0; i < DotsInUse; ++i) {
            if (GravityMode) {
              BitDotArr[i].moveDot(x, y);
            }
            BitDotArr[i].displayDot();
          }
          refreshDots();
          delay(1);
        }

    private:
        BitDots BitDotArr[BD_NUM];
        RTC_DS1307 RTC;
        SixteenBitWhiteClock Clock16Bit;
        SixteenBitMultiColClock ColorClock16Bit;
        ControlBoard Controller;
        ADXL313 Accelerometer;
        bool GravityMode = false;
        int16_t x;
        int16_t y;
        uint8_t DotsInUse = BD_NUM;
        uint16_t tippingPoint = TIP_POINT;
        uint32_t GravityModeTimer = 0;
        uint32_t TimeLimit = RESET;
        int8_t BackGIndex = 1;
        BackGround BackGArr[BACKG_NUM];

        void readInputs() {
          if(Accelerometer.dataReady()) {
            Accelerometer.readAccel(); // read all 3 axis, they are stored in class variables: myAdxl.x, myAdxl.y and myAdxl.z
          }
          x = Accelerometer.x;
          y = Accelerometer.y;
          Serial.println(Accelerometer.x);
          BitDotArr[0].ALL_DOT_BRIGHTNESS(Controller.getPRReading());
        }

        void manageGravityMode() {
          if (pow(y, 2) > tippingPoint) {
            GravityMode = true;
            GravityModeTimer = 1;
          } else {
            GravityModeTimer++;
          }
          if (!isAClock() && GravityModeTimer > TimeLimit) {
            GravityMode = false;
            resetClock();
          }
        }

        void manageDisplayMode() {
          switch(Controller.getMode()) {
            case 0:
              Clock16Bit.updateTime(BitDotArr, RTC.now());
              DotsInUse = Clock16Bit.requestNumOfDots();
              break;
            
            case 1:
              ColorClock16Bit.updateTime(BitDotArr, RTC.now());
              DotsInUse = Clock16Bit.requestNumOfDots();
              break;

            case 2:
              break;
          }
        }

        void manageController() {
          switch (Controller.buttonCheck()) {
            case Action::ModeChange:
              switch (Controller.getMode()) {
                case 0:
                  Clock16Bit.buildClock(BitDotArr);
                  break;
                case 1:
                  ColorClock16Bit.buildClock(BitDotArr);
                  break;
                case 2:
                  Clock16Bit.buildClock(BitDotArr);
                  break;
              }
              break;
            
            case Action::TimeAdjust:
              break;
          
            case Action::DoNothing:
              break;
          }
        }

        bool isAClock() {
          for (int i = 0; i < DotsInUse; ++i) {
            if (!BitDotArr[i].isInClockFormation()) {
              return false;
            }
          }
          return true;
        }

        void resetClock() {
          for (int i = 0; i < DotsInUse; ++i) {
            BitDotArr[i].setToClockPosition();
          }
        }

        void refreshDots() {
          BitDotArr[0].DISPLAY_BACKGROUND(BackGArr[BackGIndex]);
          BitDotArr[0].SHOW_ALL_DOTS();
          BitDotArr[0].CLEAR_ALL_DOTS();
        }

        void createBackGrounds() {
          BackGArr[0].createBackGround(&FireBackGround, 5);
          BackGArr[1].createBackGround(CRGB(15, 25, 5));
        }
};


CompleteClock TheClock(BUTT_1, BUTT_2, BUTT_3, PR);

void setup() {
    pinMode(BUTT_1, INPUT_PULLUP);
    pinMode(BUTT_2, INPUT_PULLUP);
    pinMode(BUTT_3, INPUT_PULLUP);
    Wire.begin();
    TheClock.begin();
    Serial.begin(115200);
}

void loop() {
  TheClock.runClock();
}