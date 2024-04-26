#include "display.h"
#include "app/app.h"

// screens
#include "screens/WordProcessor/WordProcessor.h"
#include "screens/ErrorScreen/ErrorScreen.h"
#include "screens/Menu/Menu.h"
#include "screens/WakeUp/WakeUp.h"
#include "screens/USBDRIVE/USBDRIVE.h"

// Invoke library, pins defined in platformio.ini
TFT_eSPI tft = TFT_eSPI();
U8g2_for_TFT_eSPI u8f; // U8g2 font instance

//
void display_setup()
{
  // Initialise the TFT screen
  tft.begin();

  // connect u8g2 procedures to TFT_eSPI
  u8f.begin(tft);

  //
  tft.setRotation(3);

  // Fill screen with grey so we can see the effect of printing with and without
  // a background colour defined
  tft.fillScreen(TFT_BLACK);

  // set the initial screen to word processor
  JsonDocument &app = app_status();

  // check SD card status
  if (app["SD"].as<bool>() != true)
  {
    app["screen"] = ERRORSCREEN;
  }
  else
  {
    app["screen"] = WAKEUPSCREEN;
  }
}

//
int screen_prev = -1;
void display_loop()
{
  static unsigned int last = millis();
  if (millis() - last > 100)
  {
    last = millis();

    JsonDocument &app = app_status();
    int screen = app["screen"].as<int>();

    // WORD PROCESSOR
    if (screen == WORDPROCESSOR)
    {
      // setup only once
      if (screen != screen_prev)
        WordProcessor::getInstance(&tft, &u8f).setup();

      // loop
      WordProcessor::getInstance(&tft, &u8f).render();
    }

    // ERROR SCREEN
    else if (screen == ERRORSCREEN)
    {
      // setup only once
      if (screen != screen_prev)
        ErrorScreen_setup(&tft, &u8f);

      // loop
      ErrorScreen_render(&tft, &u8f);
    }

    // MENU SCREEN
    else if (screen == MENUSCREEN)
    {
      // setup only once
      if (screen != screen_prev)
        Menu_setup(&tft, &u8f);

      // loop
      Menu_render(&tft, &u8f);
    }

    // WAKEUP SCREEN
    else if (screen == WAKEUPSCREEN)
    {
      // setup only once
      if (screen != screen_prev)
        WakeUp_setup(&tft, &u8f, true);

      // loop
      WakeUp_render(&tft, &u8f);
    }

    // SLEEP SCREEN
    else if (screen == SLEEPSCREEN)
    {
      // setup only once
      if (screen != screen_prev)
        WakeUp_setup(&tft, &u8f, false);

      // loop
      WakeUp_render(&tft, &u8f);
    }

    // USB DRIVE
    else if (screen == USBDRIVESCREEN)
    {
      // setup only once
      if (screen != screen_prev)
        USBDRIVE_setup(&tft, &u8f);

      // loop
      USBDRIVE_render(&tft, &u8f);
    }

    //
    screen_prev = screen;
  }
}
