#include "oled.h"
#include <string.h>
#include "../rsp.h"
#include "delay.h"

const uint8_t logo[] =
  "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x80\xc0\xc0\x80\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x80\xc0\xc0\xe0\xe0\xe0\xe0\xc0\xc0\x80\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xe0\xe0\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" \
  "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x80\x80\xc0\x00\x00\x00\x00\x00\x00\x80\xf3\xff\x7f\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xf0\xfc\x7e\x1f\x07\x03\x01\x00\x00\x00\x00\x01\x03\x07\x1f\xfe\xf8\xe0\x00\x00\x00\x00\x00\xf8\xf8\x18\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x3c\x7c\xf0\xe0\xc0\x80\x00\x00\x00\x00\xf0\xf0\xe0\x80\xc0\xe0\xf8\x3f\x1f\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" \
  "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x70\x78\xf0\xe0\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc3\xe7\xff\x7e\x78\xf8\xfc\x9f\x0f\x03\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x3f\xff\xff\x80\x00\x00\x00\x00\xe0\xe0\xe0\x00\x00\x00\x00\x80\xff\xff\x7f\x00\x00\x00\x00\x00\x07\x1f\x3e\x78\x70\xe0\xc0\xc0\x80\x80\x80\x80\x80\x80\x80\xc0\xc0\xe0\xf8\x7c\x19\x01\x03\x03\x03\x03\x03\x03\x07\x1f\xff\xf9\xe0\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" \
  "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01\x01\x01\x01\x01\x01\x01\x01\x01\x00\x00\x00\x00\x01\x03\x0f\x1f\x3c\xf8\xf0\xc0\x80\x00\x00\x00\x00\x00\x00\x00\x00\x03\x0f\x1f\x3e\x78\x70\xe0\xff\xff\xe1\xe0\x70\x78\x3c\x1f\x0f\x03\x00\x00\x00\x00\x00\x00\x00\x00\x80\xe0\xf8\xff\x1f\x07\x01\x03\x03\x03\x03\x03\x01\x01\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x07\x1f\xff\xfc\xe0\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" \
  "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01\x07\x0f\x1e\x7c\xf8\xe0\xc0\x00\x00\x00\x00\x00\x00\x00\x00\x00\xff\xff\xff\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x80\xf0\xfc\x7f\x1f\x03\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x03\x1f\xff\xfc\xe0\x80\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" \
  "\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x63\x67\x7f\x7e\x7c\x70\x60\x60\x60\x60\x60\x7f\x7f\x63\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x70\x7e\x7f\x6f\x61\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x63\x7f\x7f\x7c\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60" \
  "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xf0\x90\x90\x90\x90\x60\x00\x00\xf0\x00\x00\x10\x10\xf0\x10\x10\x00\xe0\x10\x10\x10\x10\x20\x00\x00\xf0\x80\x80\x80\x80\xf0\x00\x00\xf0\x90\x90\x90\x10\x00\xe0\x10\x10\x10\x10\xe0\x00\x00\xf0\x90\x90\x90\x90\x60\x00\x00\xf0\x80\x80\x40\x20\x10\x00\x00\xf0\x00\x00\xf0\x00\x00\xf0\xd0\xd0\xd0\xd0\x90\x00\x00\xf0\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" \
  "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x07\x01\x01\x01\x01\x00\x00\x00\x07\x00\x00\x00\x00\x07\x00\x00\x00\x03\x04\x04\x04\x04\x02\x00\x00\x07\x01\x01\x01\x01\x07\x00\x00\x07\x01\x01\x01\x00\x00\x03\x04\x04\x04\x04\x03\x00\x00\x07\x01\x01\x01\x02\x04\x00\x00\x07\x01\x01\x01\x02\x04\x00\x00\x04\x00\x00\x04\x00\x00\x04\x04\x04\x04\x04\x03\x00\x00\x04\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" \
  ;

void test(void) {
  //rsp_detach();
  OLED_Init();

  rsp_dump(logo, sizeof(logo));

  /* while(1){ */
  /*   rsp_dump((uint8_t*) "hello", 5); */
  /*   rsp_detach(); */
  /*   SDIN(1); */
  /*   uDelay(50); */
  /*   SDIN(0); */
  /*   uDelay(50); */
  /* } */
  while(1){

  //oled_clear();
  oled_cmd(0xa7);//--set inverted display
  memcpy(frame_buffer, logo, 1024);
  oled_refresh();
  //mDelay(500);
  rsp_detach();

  //while(1) {
  //int ii=0;
  //for(ii=2;ii>0;ii--) {
  //oled_clear();
  oled_cmd(0xa7);//--set inverted display
  //memcpy(frame_buffer, logo, 1024);
  oled_refresh();
  mDelay(500);
  oled_cmd(0xa6);//--set normal display
  //oled_clear();
  mDelay(500);
  //}

  oled_clear();
  oled_print(0, 0, "PITCHFORK!!5!", Font_8x8);
  oled_print(0, 9, "PITCHFORK!!5!", Font_8x8);
  oled_print(0,18, "PITCHFORK!!5!", Font_8x8);
  oled_print(0,27, "PITCHFORK!!5!", Font_8x8);
  oled_print(0,36, "PITCHFORK!!5!", Font_8x8);
  oled_print(0,45, "PITCHFORK!!5!", Font_8x8);
  oled_print(0,54, "PITCHFORK!!5!", Font_8x8);
  mDelay(500);

  //oled_cmd(0x81); //--set contrast control register
  //oled_cmd(0xff);
  //oled_cmd(0x29); //-- v/h scroll
  //oled_cmd(0x0);
  //oled_cmd(0x0);
  //oled_cmd(0x0);
  //oled_cmd(0x7);  // page7 is end
  //oled_cmd(0x1);  // v scroll offset 1
  //oled_cmd(0x2f); // activate scroll

  //rsp_dump(frame_buffer,256);
  // horizontal line
  //uint8_t x,y;
  //while(1) {
  //for(ii=2;ii>0;ii--) {
  //  for(y=0;y<64;y++) {
  //    for(x=0;x<128;x++) {
  //      //oled_setpixel(x, y+1);
  //      oled_setpixel(x, y);
  //      oled_delpixel(x, y-1);
  //    }
  //    oled_refresh();
  //    mDelay(5);
  //  }
  //  for(y=0;y<64;y++) {
  //    for(x=0;x<128;x++) {
  //      oled_setpixel(x, (63-y)-1);
  //      //oled_setpixel(x, 63-y);
  //      oled_delpixel(x, (63-y)+1);
  //    }
  //    oled_refresh();
  //    mDelay(5);
  //  }
  //}

  // vertical line
  uint8_t i,j,w;
  j=0;
  //for(ii=20;ii>0;ii--) {
  for(j=0;j<0x80-8;j++) {
    for(i=0;i<64;i++) {
      for(w=0;w<8;w++) {
        //oled_delpixel((j-1+w)&0x7f,16+i);
        //oled_setpixel((j+w)&0x7f,16+i);
        oled_delpixel((j-1+w)&0x7f,i);
        oled_setpixel((j+w)&0x7f,i);
      }
    }
    //j=(j+1) & 0x7f;
    oled_refresh();
    //uDelay(1);
  }
  oled_clear();
  for(j=0x80-8;j>0;j--) {
    for(i=0;i<64;i++) {
      for(w=0;w<8;w++) {
        //oled_delpixel((j-1+w)&0x7f,16+i);
        //oled_setpixel((j+w)&0x7f,16+i);
        oled_setpixel((j-1-w)&0x7f,i);
        oled_delpixel((j-w)&0x7f,i);
      }
    }
    //j=(j+1) & 0x7f;
    oled_refresh();
    //uDelay(1);
  }
  oled_clear();
  // dot
  // running pixel
  //uint8_t x,y;
  //while(1) {
  //  for(y=0;y<64;y++) {
  //    for(x=0;x<128;x++) {
  //      if(x==0)
  //        oled_delpixel(0,y-1);
  //      else
  //        oled_delpixel(x-1,y);
  //      oled_setpixel(x,y);
  //      oled_refresh();
  //      mDelay(1);
  //    }
  //    oled_delpixel(127,y);
  //    y++;
  //    for(x=0;x<128;x++) {
  //      //oled_delpixel((127-x)+1,y);
  //      oled_setpixel(127-x,y);
  //      oled_refresh();
  //      mDelay(1);
  //    }
  //  }
  //}
  // horizontal line
  //uint8_t x,y;
  //while(1) {
  //  for(y=0;y<64;y++) {
  //    for(x=0;x<128;x++) {
  //      oled_setpixel(x,y);
  //    }
  //  }
  //  oled_refresh();
  //  mDelay(1000);
  //  oled_clear();
  //  for(y=0;y<64;y+=2) {
  //    for(x=0;x<128;x++) {
  //      oled_setpixel(x,y);
  //    }
  //  }
  //  oled_refresh();
  //  mDelay(1000);
  //  oled_cmd(0xa7);//--set inverted display
  //  mDelay(1000);
  //  oled_cmd(0xa6);//--set inverted display
  //  oled_clear();
  //  /* oled_cmd(0xa6);//--set normal display */
  //  for(y=1;y<64;y+=2) {
  //    for(x=0;x<128;x++) {
  //      oled_setpixel(x,y);
  //    }
  //  }
  //  oled_refresh();
  //  oled_cmd(0xa7);//--set inverted display
  //  mDelay(1000);
  //  oled_clear();
  //  oled_cmd(0xa6);//--set inverted display
  //  for(y=0;y<64;y++) {
  //    for(x=0;x<128;x++) {
  //      oled_setpixel(x, y+1);
  //      oled_delpixel(x, y-1);
  //    }
  //    oled_refresh();
  //    mDelay(25);
  //  }
  //}
  }

  rsp_finish();
}