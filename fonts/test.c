#include <stdint.h>
#include "stm32f.h"
#include "delay.h"
#include "../rsp.h"

#include <string.h>
#include "smallfonts.h"

#define DCPIN 4
#define CSPIN 5
#define SDCLKPIN 0
#define SDINPIN 1
#define RSTPIN 11

#define DC(x)					x ? (gpio_set(GPIOD_BASE, 1 << DCPIN)) : (gpio_reset(GPIOD_BASE, 1 << DCPIN));
#define CS(x)					x ? (gpio_set(GPIOD_BASE, 1 << CSPIN)) : (gpio_reset(GPIOD_BASE, 1 << CSPIN));
#define SCLK(x)				x ? (gpio_set(GPIOD_BASE, 1 << SDCLKPIN)) : (gpio_reset(GPIOD_BASE, 1 << SDCLKPIN));
#define SDIN(x)				x ? (gpio_set(GPIOD_BASE, 1 << SDINPIN)) : (gpio_reset(GPIOD_BASE, 1 << SDINPIN));
#define RST(x)					x ? (gpio_set(GPIOE_BASE, 1 << RSTPIN)) : (gpio_reset(GPIOE_BASE, 1 << RSTPIN));

uint8_t buffer[128 * 64 / 8];

const uint8_t logo[] = "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x80\xc0\xc0\x80\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x80\xc0\xc0\xe0\xe0\xe0\xe0\xc0\xc0\x80\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xe0\xe0\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" \
  "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x80\x80\xc0\x00\x00\x00\x00\x00\x00\x80\xf3\xff\x7f\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xf0\xfc\x7e\x1f\x07\x03\x01\x00\x00\x00\x00\x01\x03\x07\x1f\xfe\xf8\xe0\x00\x00\x00\x00\x00\xf8\xf8\x18\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x3c\x7c\xf0\xe0\xc0\x80\x00\x00\x00\x00\xf0\xf0\xe0\x80\xc0\xe0\xf8\x3f\x1f\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" \
  "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x70\x78\xf0\xe0\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc3\xe7\xff\x7e\x78\xf8\xfc\x9f\x0f\x03\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x3f\xff\xff\x80\x00\x00\x00\x00\xe0\xe0\xe0\x00\x00\x00\x00\x80\xff\xff\x7f\x00\x00\x00\x00\x00\x07\x1f\x3e\x78\x70\xe0\xc0\xc0\x80\x80\x80\x80\x80\x80\x80\xc0\xc0\xe0\xf8\x7c\x19\x01\x03\x03\x03\x03\x03\x03\x07\x1f\xff\xf9\xe0\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" \
  "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01\x01\x01\x01\x01\x01\x01\x01\x01\x00\x00\x00\x00\x01\x03\x0f\x1f\x3c\xf8\xf0\xc0\x80\x00\x00\x00\x00\x00\x00\x00\x00\x03\x0f\x1f\x3e\x78\x70\xe0\xff\xff\xe1\xe0\x70\x78\x3c\x1f\x0f\x03\x00\x00\x00\x00\x00\x00\x00\x00\x80\xe0\xf8\xff\x1f\x07\x01\x03\x03\x03\x03\x03\x01\x01\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x07\x1f\xff\xfc\xe0\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" \
  "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01\x07\x0f\x1e\x7c\xf8\xe0\xc0\x00\x00\x00\x00\x00\x00\x00\x00\x00\xff\xff\xff\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x80\xf0\xfc\x7f\x1f\x03\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x03\x1f\xff\xfc\xe0\x80\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" \
  "\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x63\x67\x7f\x7e\x7c\x70\x60\x60\x60\x60\x60\x7f\x7f\x63\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x70\x7e\x7f\x6f\x61\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x63\x7f\x7f\x7c\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60" \
  "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xf0\x90\x90\x90\x90\x60\x00\x00\xf0\x00\x00\x10\x10\xf0\x10\x10\x00\xe0\x10\x10\x10\x10\x20\x00\x00\xf0\x80\x80\x80\x80\xf0\x00\x00\xf0\x90\x90\x90\x10\x00\xe0\x10\x10\x10\x10\xe0\x00\x00\xf0\x90\x90\x90\x90\x60\x00\x00\xf0\x80\x80\x40\x20\x10\x00\x00\xf0\x00\x00\xf0\x00\x00\xf0\xd0\xd0\xd0\xd0\x90\x00\x00\xf0\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" \
  "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x07\x01\x01\x01\x01\x00\x00\x00\x07\x00\x00\x00\x00\x07\x00\x00\x00\x03\x04\x04\x04\x04\x02\x00\x00\x07\x01\x01\x01\x01\x07\x00\x00\x07\x01\x01\x01\x00\x00\x03\x04\x04\x04\x04\x03\x00\x00\x07\x01\x01\x01\x02\x04\x00\x00\x07\x01\x01\x01\x02\x04\x00\x00\x04\x00\x00\x04\x00\x00\x04\x04\x04\x04\x04\x03\x00\x00\x04\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00";

static void send(uint8_t byte) {
  int8_t i;
  SCLK(1);
  for (i=7; i>=0; i--) {
    SCLK(0);
    SDIN(byte & (1 << i));
    SCLK(1);
  }
}

/**
 * @brief  Writes command to the LCD
 * @param LCD_Reg: address of the selected register.
 * @arg LCD_RegValue: value to write to the selected register.
 * @retval : None
 */
static void oled_cmd(uint8_t reg) {
  CS(1);
  DC(0);
  CS(0);
  send(reg);
  CS(1);
}

static void oled_data(uint8_t data) {
  CS(1);
  DC(1);
  CS(0);
  send(data);
  CS(1);
}

static void oled_refresh(void) {
  oled_cmd(0x00);//---set low column address
  oled_cmd(0x10);//---set high column address
  oled_cmd(0x40);//--set start line address

  uint16_t i;
  for (i=0; i<1024; i++) {
    oled_data(buffer[i]);
  }
}

void oled_clear(void) {
  memset(buffer, 0, 1024);
  oled_refresh();
}

static void oled_setpixel(uint8_t x, uint8_t y) {
  if ((x >= 128) || (y >= 64))
    return;

  buffer[x+ (y/8)*128] |= (1 << y%8);
}

/* static void oled_delpixel(uint8_t x, uint8_t y) { */
/*   if ((x >= 128) || (y >= 64)) */
/*     return; */

/*   buffer[x+ (y/8)*128] &= ~(1 << y%8); */
/* } */

const uint8_t Font[][5] = { // Refer to "Times New Roman" Font Database... 5 x 7 font
  { 0x00,0x00,0x00,0x00,0x00},
  { 0x00,0x00,0x4F,0x00,0x00}, //   (  1)  ! - 0x0021 Exclamation Mark
  { 0x00,0x07,0x00,0x07,0x00}, //   (  2)  " - 0x0022 Quotation Mark
  { 0x14,0x7F,0x14,0x7F,0x14}, //   (  3)  # - 0x0023 Number Sign
  { 0x24,0x2A,0x7F,0x2A,0x12}, //   (  4)  $ - 0x0024 Dollar Sign
  { 0x23,0x13,0x08,0x64,0x62}, //   (  5)  % - 0x0025 Percent Sign
  { 0x36,0x49,0x55,0x22,0x50}, //   (  6)  & - 0x0026 Ampersand
  { 0x00,0x05,0x03,0x00,0x00}, //   (  7)  ' - 0x0027 Apostrophe
  { 0x00,0x1C,0x22,0x41,0x00}, //   (  8)  ( - 0x0028 Left Parenthesis
  { 0x00,0x41,0x22,0x1C,0x00}, //   (  9)  ) - 0x0029 Right Parenthesis
  { 0x14,0x08,0x3E,0x08,0x14}, //   ( 10)  * - 0x002A Asterisk
  { 0x08,0x08,0x3E,0x08,0x08}, //   ( 11)  + - 0x002B Plus Sign
  { 0x00,0x50,0x30,0x00,0x00}, //   ( 12)  , - 0x002C Comma
  { 0x08,0x08,0x08,0x08,0x08}, //   ( 13)  - - 0x002D Hyphen-Minus
  { 0x00,0x60,0x60,0x00,0x00}, //   ( 14)  . - 0x002E Full Stop
  { 0x20,0x10,0x08,0x04,0x02}, //   ( 15)  / - 0x002F Solidus
  { 0x3E,0x51,0x49,0x45,0x3E}, //   ( 16)  0 - 0x0030 Digit Zero
  { 0x00,0x42,0x7F,0x40,0x00}, //   ( 17)  1 - 0x0031 Digit One
  { 0x42,0x61,0x51,0x49,0x46}, //   ( 18)  2 - 0x0032 Digit Two
  { 0x21,0x41,0x45,0x4B,0x31}, //   ( 19)  3 - 0x0033 Digit Three
  { 0x18,0x14,0x12,0x7F,0x10}, //   ( 20)  4 - 0x0034 Digit Four
  { 0x27,0x45,0x45,0x45,0x39}, //   ( 21)  5 - 0x0035 Digit Five
  { 0x3C,0x4A,0x49,0x49,0x30}, //   ( 22)  6 - 0x0036 Digit Six
  { 0x01,0x71,0x09,0x05,0x03}, //   ( 23)  7 - 0x0037 Digit Seven
  { 0x36,0x49,0x49,0x49,0x36}, //   ( 24)  8 - 0x0038 Digit Eight
  { 0x06,0x49,0x49,0x29,0x1E}, //   ( 25)  9 - 0x0039 Dight Nine
  { 0x00,0x36,0x36,0x00,0x00}, //   ( 26)  : - 0x003A Colon
  { 0x00,0x56,0x36,0x00,0x00}, //   ( 27)  ; - 0x003B Semicolon
  { 0x08,0x14,0x22,0x41,0x00}, //   ( 28)  < - 0x003C Less-Than Sign
  { 0x14,0x14,0x14,0x14,0x14}, //   ( 29)  = - 0x003D Equals Sign
  { 0x00,0x41,0x22,0x14,0x08}, //   ( 30)  > - 0x003E Greater-Than Sign
  { 0x02,0x01,0x51,0x09,0x06}, //   ( 31)  ? - 0x003F Question Mark
  { 0x32,0x49,0x79,0x41,0x3E}, //   ( 32)  @ - 0x0040 Commercial At
  { 0x7E,0x11,0x11,0x11,0x7E}, //   ( 33)  A - 0x0041 Latin Capital Letter A
  { 0x7F,0x49,0x49,0x49,0x36}, //   ( 34)  B - 0x0042 Latin Capital Letter B
  { 0x3E,0x41,0x41,0x41,0x22}, //   ( 35)  C - 0x0043 Latin Capital Letter C
  { 0x7F,0x41,0x41,0x22,0x1C}, //   ( 36)  D - 0x0044 Latin Capital Letter D
  { 0x7F,0x49,0x49,0x49,0x41}, //   ( 37)  E - 0x0045 Latin Capital Letter E
  { 0x7F,0x09,0x09,0x09,0x01}, //   ( 38)  F - 0x0046 Latin Capital Letter F
  { 0x3E,0x41,0x49,0x49,0x7A}, //   ( 39)  G - 0x0047 Latin Capital Letter G
  { 0x7F,0x08,0x08,0x08,0x7F}, //   ( 40)  H - 0x0048 Latin Capital Letter H
  { 0x00,0x41,0x7F,0x41,0x00}, //   ( 41)  I - 0x0049 Latin Capital Letter I
  { 0x20,0x40,0x41,0x3F,0x01}, //   ( 42)  J - 0x004A Latin Capital Letter J
  { 0x7F,0x08,0x14,0x22,0x41}, //   ( 43)  K - 0x004B Latin Capital Letter K
  { 0x7F,0x40,0x40,0x40,0x40}, //   ( 44)  L - 0x004C Latin Capital Letter L
  { 0x7F,0x02,0x0C,0x02,0x7F}, //   ( 45)  M - 0x004D Latin Capital Letter M
  { 0x7F,0x04,0x08,0x10,0x7F}, //   ( 46)  N - 0x004E Latin Capital Letter N
  { 0x3E,0x41,0x41,0x41,0x3E}, //   ( 47)  O - 0x004F Latin Capital Letter O
  { 0x7F,0x09,0x09,0x09,0x06}, //   ( 48)  P - 0x0050 Latin Capital Letter P
  { 0x3E,0x41,0x51,0x21,0x5E}, //   ( 49)  Q - 0x0051 Latin Capital Letter Q
  { 0x7F,0x09,0x19,0x29,0x46}, //   ( 50)  R - 0x0052 Latin Capital Letter R
  { 0x46,0x49,0x49,0x49,0x31}, //   ( 51)  S - 0x0053 Latin Capital Letter S
  { 0x01,0x01,0x7F,0x01,0x01}, //   ( 52)  T - 0x0054 Latin Capital Letter T
  { 0x3F,0x40,0x40,0x40,0x3F}, //   ( 53)  U - 0x0055 Latin Capital Letter U
  { 0x1F,0x20,0x40,0x20,0x1F}, //   ( 54)  V - 0x0056 Latin Capital Letter V
  { 0x3F,0x40,0x38,0x40,0x3F}, //   ( 55)  W - 0x0057 Latin Capital Letter W
  { 0x63,0x14,0x08,0x14,0x63}, //   ( 56)  X - 0x0058 Latin Capital Letter X
  { 0x07,0x08,0x70,0x08,0x07}, //   ( 57)  Y - 0x0059 Latin Capital Letter Y
  { 0x61,0x51,0x49,0x45,0x43}, //   ( 58)  Z - 0x005A Latin Capital Letter Z
  { 0x00,0x7F,0x41,0x41,0x00}, //   ( 59)  [ - 0x005B Left Square Bracket
  { 0x02,0x04,0x08,0x10,0x20}, //   ( 60)  \ - 0x005C Reverse Solidus
  { 0x00,0x41,0x41,0x7F,0x00}, //   ( 61)  ] - 0x005D Right Square Bracket
  { 0x04,0x02,0x01,0x02,0x04}, //   ( 62)  ^ - 0x005E Circumflex Accent
  { 0x40,0x40,0x40,0x40,0x40}, //   ( 63)  _ - 0x005F Low Line
  { 0x01,0x02,0x04,0x00,0x00}, //   ( 64)  ` - 0x0060 Grave Accent
  { 0x20,0x54,0x54,0x54,0x78}, //   ( 65)  a - 0x0061 Latin Small Letter A
  { 0x7F,0x48,0x44,0x44,0x38}, //   ( 66)  b - 0x0062 Latin Small Letter B
  { 0x38,0x44,0x44,0x44,0x20}, //   ( 67)  c - 0x0063 Latin Small Letter C
  { 0x38,0x44,0x44,0x48,0x7F}, //   ( 68)  d - 0x0064 Latin Small Letter D
  { 0x38,0x54,0x54,0x54,0x18}, //   ( 69)  e - 0x0065 Latin Small Letter E
  { 0x08,0x7E,0x09,0x01,0x02}, //   ( 70)  f - 0x0066 Latin Small Letter F
  { 0x06,0x49,0x49,0x49,0x3F}, //   ( 71)  g - 0x0067 Latin Small Letter G
  { 0x7F,0x08,0x04,0x04,0x78}, //   ( 72)  h - 0x0068 Latin Small Letter H
  { 0x00,0x44,0x7D,0x40,0x00}, //   ( 73)  i - 0x0069 Latin Small Letter I
  { 0x20,0x40,0x44,0x3D,0x00}, //   ( 74)  j - 0x006A Latin Small Letter J
  { 0x7F,0x10,0x28,0x44,0x00}, //   ( 75)  k - 0x006B Latin Small Letter K
  { 0x00,0x41,0x7F,0x40,0x00}, //   ( 76)  l - 0x006C Latin Small Letter L
  { 0x7C,0x04,0x18,0x04,0x7C}, //   ( 77)  m - 0x006D Latin Small Letter M
  { 0x7C,0x08,0x04,0x04,0x78}, //   ( 78)  n - 0x006E Latin Small Letter N
  { 0x38,0x44,0x44,0x44,0x38}, //   ( 79)  o - 0x006F Latin Small Letter O
  { 0x7C,0x14,0x14,0x14,0x08}, //   ( 80)  p - 0x0070 Latin Small Letter P
  { 0x08,0x14,0x14,0x18,0x7C}, //   ( 81)  q - 0x0071 Latin Small Letter Q
  { 0x7C,0x08,0x04,0x04,0x08}, //   ( 82)  r - 0x0072 Latin Small Letter R
  { 0x48,0x54,0x54,0x54,0x20}, //   ( 83)  s - 0x0073 Latin Small Letter S
  { 0x04,0x3F,0x44,0x40,0x20}, //   ( 84)  t - 0x0074 Latin Small Letter T
  { 0x3C,0x40,0x40,0x20,0x7C}, //   ( 85)  u - 0x0075 Latin Small Letter U
  { 0x1C,0x20,0x40,0x20,0x1C}, //   ( 86)  v - 0x0076 Latin Small Letter V
  { 0x3C,0x40,0x30,0x40,0x3C}, //   ( 87)  w - 0x0077 Latin Small Letter W
  { 0x44,0x28,0x10,0x28,0x44}, //   ( 88)  x - 0x0078 Latin Small Letter X
  { 0x0C,0x50,0x50,0x50,0x3C}, //   ( 89)  y - 0x0079 Latin Small Letter Y
  { 0x44,0x64,0x54,0x4C,0x44}, //   ( 90)  z - 0x007A Latin Small Letter Z
  { 0x00,0x08,0x36,0x41,0x00}, //   ( 91)  { - 0x007B Left Curly Bracket
  { 0x00,0x00,0x7F,0x00,0x00}, //   ( 92)  | - 0x007C Vertical Line
  { 0x00,0x41,0x36,0x08,0x00}, //   ( 93)  } - 0x007D Right Curly Bracket
  { 0x02,0x01,0x02,0x04,0x02}, //   ( 94)  ~ - 0x007E Tilde
  { 0x3E,0x55,0x55,0x41,0x22}, //   ( 95)  C - 0x0080 <Control>
  { 0x00,0x00,0x00,0x00,0x00}, //   ( 96)    - 0x00A0 No-Break Space
  { 0x00,0x00,0x79,0x00,0x00}, //   ( 97)  ! - 0x00A1 Inverted Exclamation Mark
  { 0x18,0x24,0x74,0x2E,0x24}, //   ( 98)  c - 0x00A2 Cent Sign
  { 0x48,0x7E,0x49,0x42,0x40}, //   ( 99)  L - 0x00A3 Pound Sign
  { 0x5D,0x22,0x22,0x22,0x5D}, //   (100)  o - 0x00A4 Currency Sign
  { 0x15,0x16,0x7C,0x16,0x15}, //   (101)  Y - 0x00A5 Yen Sign
  { 0x00,0x00,0x77,0x00,0x00}, //   (102)  | - 0x00A6 Broken Bar
  { 0x0A,0x55,0x55,0x55,0x28}, //   (103)    - 0x00A7 Section Sign
  { 0x00,0x01,0x00,0x01,0x00}, //   (104)  " - 0x00A8 Diaeresis
  { 0x00,0x0A,0x0D,0x0A,0x04}, //   (105)    - 0x00AA Feminine Ordinal Indicator
  { 0x08,0x14,0x2A,0x14,0x22}, //   (106) << - 0x00AB Left-Pointing Double Angle Quotation Mark
  { 0x04,0x04,0x04,0x04,0x1C}, //   (107)    - 0x00AC Not Sign
  { 0x00,0x08,0x08,0x08,0x00}, //   (108)  - - 0x00AD Soft Hyphen
  { 0x01,0x01,0x01,0x01,0x01}, //   (109)    - 0x00AF Macron
  { 0x00,0x02,0x05,0x02,0x00}, //   (110)    - 0x00B0 Degree Sign
  { 0x44,0x44,0x5F,0x44,0x44}, //   (111) +- - 0x00B1 Plus-Minus Sign
  { 0x00,0x00,0x04,0x02,0x01}, //   (112)  ` - 0x00B4 Acute Accent
  { 0x7E,0x20,0x20,0x10,0x3E}, //   (113)  u - 0x00B5 Micro Sign
  { 0x06,0x0F,0x7F,0x00,0x7F}, //   (114)    - 0x00B6 Pilcrow Sign
  { 0x00,0x18,0x18,0x00,0x00}, //   (115)  . - 0x00B7 Middle Dot
  { 0x00,0x40,0x50,0x20,0x00}, //   (116)    - 0x00B8 Cedilla
  { 0x00,0x0A,0x0D,0x0A,0x00}, //   (117)    - 0x00BA Masculine Ordinal Indicator
  { 0x22,0x14,0x2A,0x14,0x08}, //   (118) >> - 0x00BB Right-Pointing Double Angle Quotation Mark
  { 0x17,0x08,0x34,0x2A,0x7D}, //   (119) /4 - 0x00BC Vulgar Fraction One Quarter
  { 0x17,0x08,0x04,0x6A,0x59}, //   (120) /2 - 0x00BD Vulgar Fraction One Half
  { 0x30,0x48,0x45,0x40,0x20}, //   (121)  ? - 0x00BE Inverted Question Mark
  { 0x42,0x00,0x42,0x00,0x42}, //   (122)    - 0x00BF Bargraph - 0
  { 0x7E,0x42,0x00,0x42,0x00}, //   (123)    - 0x00BF Bargraph - 1
  { 0x7E,0x7E,0x00,0x42,0x00}, //   (124)    - 0x00BF Bargraph - 2
  { 0x7E,0x7E,0x7E,0x42,0x00}, //   (125)    - 0x00BF Bargraph - 3
  { 0x7E,0x7E,0x7E,0x7E,0x00}, //   (126)    - 0x00BF Bargraph - 4
  { 0x7E,0x7E,0x7E,0x7E,0x7E}, //   (127)    - 0x00BF Bargraph - 5
};

#define CHAR_FORMAT 0

void OLED_send_char(unsigned char ascii){
  unsigned char i, buffer;
  for(i=0;i<5;i++){
    buffer = Font[ascii - 32][i];
    buffer ^= CHAR_FORMAT;  // apply
    oled_data(buffer);
  }
  oled_data(CHAR_FORMAT);    // the gap
}

void oled_send_string(const char *string){  // Sends a string of chars untill null terminator
  unsigned char i=0, buffer;
  while(*string) {
    for(i=0;i<5;i++){
      buffer = Font[(*string)- 32][i];
      buffer ^= CHAR_FORMAT;
      oled_data(buffer);
    }
    oled_data(CHAR_FORMAT);    // the gap
    string++;
  }
}

static void oled_drawchar(uint16_t x, uint16_t y, uint8_t c, struct FONT_DEF font) {
  uint8_t col, column[font.u8Width];

  // Check if the requested character is available
  if ((c >= font.u8FirstChar) && (c <= font.u8LastChar)) {
    // Retrieve appropriate columns from font data
    for (col = 0; col < font.u8Width; col++) {
      column[col] = font.au8FontTable[((c - 32) * font.u8Width) + col]; // Get first column of appropriate character
    }
  } else {
    // Requested character is not available in this font ... send a space instead
    for (col = 0; col < font.u8Width; col++) {
      column[col] = 0xFF; // Send solid space
    }
  }

  // Render each column
  uint16_t xoffset, yoffset;
  for (xoffset = 0; xoffset < font.u8Width; xoffset++) {
    for (yoffset = 0; yoffset < (font.u8Height + 1); yoffset++) {
      if ((column[xoffset] >> yoffset) & 1) {
        oled_setpixel(x + xoffset, y + yoffset);
      }
    }
  }
}

void oled_print(uint16_t x, uint16_t y, char* text, struct FONT_DEF font) {
  uint8_t l;
  for (l = 0; l < strlen(text); l++) {
    oled_drawchar(x + (l * (font.u8Width + 1)), y, text[l], font);
  }
  oled_refresh();
}

static void OLED_Init(void) {      //Generated by Internal DC/DC Circuit // VCC externally supplied
  GPIO_Regs *greg;

  /* Enable FSMC, GPIOD, GPIOE clocks */
  MMIO32(RCC_AHB1ENR) |= RCC_AHB1Periph_GPIOD | RCC_AHB1Periph_GPIOE;

  /* Set PD.00(D0), PD.01(D1), PD.04(RS), PD.05(CS) as alternate function push pull */
  greg = (GPIO_Regs *) GPIOD_BASE;
  greg->MODER  &= (unsigned int) ~((3 << (0 << 1))  |
                                   (3 << (1 << 1))  |
                                   (3 << (4 << 1))  |
                                   (3 << (5 << 1))
                                   );
  greg->PUPDR  &= (unsigned int) ~((3 << (0 << 1))  |
                                   (3 << (1 << 1))  |
                                   (3 << (4 << 1))  |
                                   (3 << (5 << 1))
                                   );
  greg->MODER |=   ((GPIO_Mode_OUT << (0 << 1))  |
                    (GPIO_Mode_OUT << (1 << 1))  |
                    (GPIO_Mode_OUT << (4 << 1))  |
                    (GPIO_Mode_OUT << (5 << 1))
                    );
  greg->PUPDR |= ((GPIO_PuPd_UP << (0 << 1)) |
                  (GPIO_PuPd_UP << (1 << 1)) |
                  (GPIO_PuPd_UP << (4 << 1)) |
                  (GPIO_PuPd_UP << (5 << 1)));
  greg->OSPEEDR |= ((GPIO_Speed_100MHz << (0 << 1))  |
                    (GPIO_Speed_100MHz << (1 << 1))  |
                    (GPIO_Speed_100MHz << (4 << 1))  |
                    (GPIO_Speed_100MHz << (5 << 1))
                    );

  /* set PE.11(RST) as output */
  greg = (GPIO_Regs *) GPIOE_BASE;
  greg->MODER  &= (unsigned int) ~(3 << (11 << 1));
  greg->PUPDR  &= (unsigned int) ~(3 << (11 << 1));
  greg->MODER |= (GPIO_Mode_OUT << (11 << 1));
  greg->PUPDR |= (GPIO_PuPd_UP << (11 << 1));
  greg->OSPEEDR |= (GPIO_Speed_100MHz << (11 << 1));

  RST(1);
  uDelay(2);
  RST(0);
  mDelay(10);
  RST(1);
  uDelay(2);

  oled_cmd(0xae);//--turn off oled panel
  oled_cmd(0x20);//---set page addressing mode
  oled_cmd(0x00);// 02
  oled_cmd(0x00);//---set low column address
  oled_cmd(0x10);//---set high column address
  oled_cmd(0x40);//--set start line address
  oled_cmd(0x81);//--set contrast control register
  oled_cmd(0xcf);
  oled_cmd(0xa0);//--set segment re-map 95 to 0
  oled_cmd(0xa6);//--set normal display
  oled_cmd(0xa4);//--set display all on resume
  oled_cmd(0xa8);//--set multiplex ratio(1 to 64)
  oled_cmd(0x3f);//--1/64 duty
  oled_cmd(0xd3);//-set display offset
  oled_cmd(0x00);//-not offset
  oled_cmd(0xd5);//--set display clock divide ratio/oscillator frequency
  oled_cmd(0x80);//--set divide ratio
  oled_cmd(0xd9);//--set pre-charge period
  oled_cmd(0xf1);
  oled_cmd(0xda);//--set com pins hardware configuration
  oled_cmd(0x12);
  oled_cmd(0xdb);//--set vcomh
  oled_cmd(0x40);
  oled_cmd(0x8d);//--set Charge Pump enable/disable
  oled_cmd(0x14);//--set(0x14) enable
  oled_cmd(0xaf);//--turn on oled panel

  uDelay(100);
  oled_clear();
}

void test(void) {
  //uDelay(10);
  OLED_Init();

  //oled_cmd(0xa7);//--set inverted display
  //oled_print(32, 28, "PITCHFORK!!5!", Font_System5x8);
  //oled_print(18, 28, "PITCHFORK!!5!", Font_System7x8);
  //oled_print(0, 0, "PITCHFORK!!5!", Font_System3x6);
  memcpy(buffer, logo, 1024);
  oled_refresh();
  mDelay(5);
  oled_clear();

  //oled_print(0, 0, "PITCHFORK!!5!", Font_System5x8);
  oled_print(0, 9, "PITCHFORK!!5!", Font_System5x8);
  //oled_print(0, 18, "PITCHFORK!!5!", Font_System5x8);
  //oled_print(0, 27, "PITCHFORK!!5!", Font_System5x8);
  //oled_print(0, 0, "PITCHFORK!!5!", Font_System7x8);
  //oled_print(0, 9, "PITCHFORK!!5!", Font_System7x8);
  oled_print(0, 18, "PITCHFORK!!5!", Font_System7x8);
  //oled_print(0, 27, "PITCHFORK!!5!", Font_System7x8);
  //oled_print(0, 36, "PITCHFORK!!5!", Font_System7x8);
  oled_print(0, 30, "PITCHFORK!!5!", Font_8x8Thin);

  oled_cmd(0x00);//---set low column address
  oled_cmd(0x10);//---set high column address
  oled_cmd(0x40);//--set start line address
  oled_send_string("PITCHFORK!!5!");

  //oled_cmd(0x81); //--set contrast control register
  //oled_cmd(0xff);
  //oled_cmd(0x29); //-- v/h scroll
  //oled_cmd(0x0);
  //oled_cmd(0x0);
  //oled_cmd(0x0);
  //oled_cmd(0x7);  // page7 is end
  //oled_cmd(0x1);  // v scroll offset 1
  //oled_cmd(0x2f); // activate scroll
  //rsp_dump(buffer,256);
  //rsp_detach();
  //uint8_t i,j,w;
  //j=0;
  //while(1) {
  //  for(i=0;i<32;i++) {
  //    for(w=0;w<8;w++) {
  //      oled_delpixel((j-1+w)&0x7f,16+i);
  //      oled_setpixel((j+w)&0x7f,16+i);
  //    }
  //  }
  //  j=(j+1) & 0x7f;
  //  oled_refresh();
  //  mDelay(1);
  //}
  rsp_finish();
}
