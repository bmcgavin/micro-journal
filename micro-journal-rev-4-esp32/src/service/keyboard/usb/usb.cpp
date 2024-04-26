#include "usb.h"
#include "app/app.h"
#include "app/config/config.h"
#include "service/display/display.h"

//
#include "screens/WordProcessor/WordProcessor.h"
#include "screens/ErrorScreen/ErrorScreen.h"
#include "screens/Menu/Menu.h"
#include "screens/WakeUp/WakeUp.h"

//
#include <EspUsbHost.h>
#include "USB.h"
#include "USBMSC.h"

/*
  HID_LOCAL_NotSupported = 0   , ///< NotSupported
  HID_LOCAL_Arabic             , ///< Arabic
  HID_LOCAL_Belgian            , ///< Belgian
  HID_LOCAL_Canadian_Bilingual , ///< Canadian_Bilingual
  HID_LOCAL_Canadian_French    , ///< Canadian_French
  HID_LOCAL_Czech_Republic     , ///< Czech_Republic
  HID_LOCAL_Danish             , ///< Danish
  HID_LOCAL_Finnish            , ///< Finnish
  HID_LOCAL_French             , ///< French
  HID_LOCAL_German             , ///< German
  HID_LOCAL_Greek              , ///< Greek
  HID_LOCAL_Hebrew             , ///< Hebrew
  HID_LOCAL_Hungary            , ///< Hungary
  HID_LOCAL_International      , ///< International
  HID_LOCAL_Italian            , ///< Italian
  HID_LOCAL_Japan_Katakana     , ///< Japan_Katakana
  HID_LOCAL_Korean             , ///< Korean
  HID_LOCAL_Latin_American     , ///< Latin_American
  HID_LOCAL_Netherlands_Dutch  , ///< Netherlands/Dutch
  HID_LOCAL_Norwegian          , ///< Norwegian
  HID_LOCAL_Persian_Farsi      , ///< Persian (Farsi)
  HID_LOCAL_Poland             , ///< Poland
  HID_LOCAL_Portuguese         , ///< Portuguese
  HID_LOCAL_Russia             , ///< Russia
  HID_LOCAL_Slovakia           , ///< Slovakia
  HID_LOCAL_Spanish            , ///< Spanish
  HID_LOCAL_Swedish            , ///< Swedish
  HID_LOCAL_Swiss_French       , ///< Swiss/French
  HID_LOCAL_Swiss_German       , ///< Swiss/German
  HID_LOCAL_Switzerland        , ///< Switzerland
  HID_LOCAL_Taiwan             , ///< Taiwan
  HID_LOCAL_Turkish_Q          , ///< Turkish-Q
  HID_LOCAL_UK                 , ///< UK
  HID_LOCAL_US                 , ///< US
  HID_LOCAL_Yugoslavia         , ///< Yugoslavia
  HID_LOCAL_Turkish_F            ///< Turkish-F
*/

#include "locale/ca-bi.h"
#include "locale/it.h"
#include "locale/us.h"

class MyEspUsbHost : public EspUsbHost
{
  //
  uint8_t getKeycodeToAscii(uint8_t keycode, uint8_t shift, uint8_t altgr)
  {
    //
    static uint8_t const keyboard_conv_table_us[128][2] = {HID_KEYCODE_TO_ASCII_US};
    static uint8_t const keyboard_conv_table_it[128][3] = {HID_KEYCODE_TO_ASCII_IT};
    static uint8_t const keyboard_conv_table_ca_bi[128][5] = {HID_KEYCODE_TO_ASCII_CA_BI};

    // debug
    // app_log("%d %d %d\n", keycode, shift, altgr);

    //
    if (shift > 1)
      shift = 1;

    if (hidLocal == HID_LOCAL_Italian)
    {
      if (altgr > 0)
        shift = 2;

      // Italian
      return keyboard_conv_table_it[keycode][shift];
    }
    else if (hidLocal == HID_LOCAL_Canadian_Bilingual)
    {
      if (altgr > 0)
        shift = 2;

      // Canadian Bilingual
      return keyboard_conv_table_ca_bi[keycode][shift];
    }
    else
    {
      // US
      return keyboard_conv_table_us[keycode][shift];
    }
  }

  //
  void onKeyboardKey(uint8_t ascii, uint8_t keycode, uint8_t modifier)
  {

    if (ascii == 27)
    {
      keyboard_key(MENU);
    }

    else if (ascii != 0)
    {
      keyboard_key(ascii);
    }
  };

  void onKeyboard(hid_keyboard_report_t report, hid_keyboard_report_t last_report)
  {
    // app_log("%02x %02x %02x %02x %02x %02x\n", report.modifier, report.keycode[0], report.keycode[1], report.keycode[2], report.keycode[3], report.keycode[4], report.keycode[5]);
  }
};

MyEspUsbHost usbHost;
bool mass_drive_mode = false;

// initialize USB HOST
String keyboard_layout_prev;
void keyboard_layout(String layout)
{
  Serial.println(layout);

  if (layout == "US")
  {
    usbHost.setHIDLocal(HID_LOCAL_US);
  }

  else if (layout == "IT")
  {
    usbHost.setHIDLocal(HID_LOCAL_Italian);
  }
  else if (layout == "CA")
  {
    usbHost.setHIDLocal(HID_LOCAL_Canadian_Bilingual);
  }

  //
  keyboard_layout_prev = layout;
}

static const uint32_t DISK_SECTOR_COUNT = 2 * 8;  // 8KB is the smallest size that windows allow to mount
static const uint16_t DISK_SECTOR_SIZE = 512;     // Should be 512
static const uint16_t DISC_SECTORS_PER_TABLE = 1; // each table sector can fit 170KB (340 sectors)

void keyboard_setup()
{
  JsonDocument &app = app_status();
  if (app["config"]["drive"].as<bool>())
  {
    //
    mass_drive_mode = true;
  }

  else
  {
    //
    app_log("Init Keyboard\n");

    // usb host setup
    usbHost.begin();

    // update the locale depending on your keyboard layout
    //
    String layout = app["config"]["keyboard_layout"].as<String>();
    if (layout == "null" || layout.isEmpty())
      layout = "US"; // defaults to US layout

    keyboard_layout(layout);
  }
}

///
void keyboard_loop()
{
  if (mass_drive_mode)
  {
    // as a thumb drive don't initiate usb host mode
  }
  else
  {
    // as a usb host
    usbHost.task();

    static unsigned int last = 0;
    if (millis() - last > 1000)
    {
      //
      last = millis();

      // check if layout is changed
      JsonDocument &app = app_status();
      String layout = app["config"]["keyboard_layout"].as<String>();
      if (!layout.equals(keyboard_layout_prev))
      {
        app_log("Keyboard layout changed %s\n", layout);
        keyboard_layout(layout);
      }
    }
  }
}

void keyboard_key(char key)
{
  //
  // depending on the screen
  // send the keystrokes
  JsonDocument &app = app_status();
  int screen = app["screen"].as<int>();

  if (screen == WORDPROCESSOR)
  {
    // send the key stroke to word processor
    WordProcessor::getInstance(nullptr, nullptr).keyboard(key);
  }
  else if (screen == MENUSCREEN)
  {
    Menu_keyboard(key);
  }
  else if (screen == ERRORSCREEN)
  {
    ErrorScreen_keyboard(key);
  }
  else if (screen == WAKEUPSCREEN || screen == SLEEPSCREEN)
  {
    WakeUp_keyboard(key);
  }
}
