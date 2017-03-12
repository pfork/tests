#include <string.h>
#include <stdint.h>

#include "../rsp.h"
#include "oled.h"
#include "fips202.h"
#include "storage.h"
#include "widgets.h"
#include "master.h"
#include "pitchfork.h"
#include "crypto_generichash.h"

extern MenuCtx appctx;

static const char *seedmenuitems[]={"HOTP", "delete"};
unsigned long long counter=0;

char ltos(char *d, long x) {
  uint8_t t[11], *p=t;
  p += 11;
  *--p = 0;
  do {
    *--p = '0' + x % 10;
    x /= 10;
  } while (x);
  memcpy(d,p,11-(p-t));
  return 11-(p-t);
}

// HMAC-sha3-256 based HOTP
// expects 32 byte key
// returns 8 digits in buffer pointed by dest (allocated by caller)
void hotp(char* dest,
          const unsigned char* key,
          const unsigned long long counter) {
  unsigned char temp[32];
  unsigned char input[32 + sizeof(counter)];
  unsigned char input2[32 + sizeof(temp)];
  long s;

  memcpy(input,key,32);
  memcpy(input+32,(unsigned char*) &counter,sizeof(counter));
  sha3256(temp, input, sizeof(input));
  memset(input,0,32); // clear key from temp

  memcpy(input2,key,32);
  memcpy(input2+32,temp,32);
  sha3256(temp,input2, sizeof(input2));
  memset(input2,0,32); // clear key from temp

  unsigned int offset = temp[31] & 0x0f;

  s = (((temp[offset] & 0x7f) << 24)
       | ((temp[offset + 1] & 0xff) << 16)
       | ((temp[offset + 2] & 0xff) << 8) | ((temp[offset + 3] & 0xff)));

  s = s % 100000000;
  ltos(dest,s);
}

char listseeds_mode = 0;
unsigned char selected_seed[STORAGE_ID_LEN];

static void seedactions_cb(char menuidx) {
  if(menuidx==0) {// implement hotp
    char token[9];
    unsigned char key[32];
    get_seed(key, 0, selected_seed, 0);
    hotp(token, key, counter++);
    oled_clear();
    oled_print(0,0,token,Font_8x8);

    //appctx.idx=0; appctx.top=0;
    listseeds_mode=0;
  } else if(menuidx==1) { // implement delete
    del_seed(0, selected_seed);
  }
}

static void listseeds_cb(char menuidx) {
  unsigned int i=0, ptr = FLASH_BASE;
  unsigned char nonce[crypto_secretbox_NONCEBYTES];
  unsigned char cipher[crypto_secretbox_KEYBYTES+crypto_secretbox_ZEROBYTES];
  unsigned char plain[crypto_secretbox_KEYBYTES+crypto_secretbox_ZEROBYTES];
  UserRecord *userdata = get_userrec();
  // search for seed which is menuidx
  while(ptr < FLASH_BASE + FLASH_SECTOR_SIZE && *((unsigned char*)ptr) != EMPTY ) {
    if(*((unsigned char*)ptr) != SEED) { // only seeds
        goto endloop;
    }

    // pad ciphertext with extra 16 bytes
    memcpy(cipher+crypto_secretbox_BOXZEROBYTES,
           ((SeedRecord*) ptr)->mac,
           crypto_scalarmult_curve25519_BYTES+crypto_secretbox_MACBYTES);
    // why >>2 ?
    memset(cipher, 0, crypto_secretbox_BOXZEROBYTES>>2);
    // nonce
    crypto_generichash(nonce, crypto_secretbox_NONCEBYTES,                  // output
                       (unsigned char*) ((SeedRecord*) ptr)->peerid, STORAGE_ID_LEN<<1, // input
                       (unsigned char*) userdata->salt, USER_SALT_LEN);      // key
    // decrypt
    // todo/bug erase plain after usage, leaks keymaterial
    if(crypto_secretbox_open(plain,                 // ciphertext output
                             cipher,                // plaintext input
                             crypto_scalarmult_curve25519_BYTES+crypto_secretbox_ZEROBYTES, // plain length
                             nonce,                 // nonce
                             get_master_key())      // key
       == -1) {
      // rewind name of corrupt seed
      goto endloop;
    }
    if(i++==menuidx) {
      memcpy(selected_seed, ((SeedRecord*) ptr)->keyid, STORAGE_ID_LEN);
      break;
    }

  endloop:
    if( (ptr = next_rec(ptr)) == -1) {
      // invalid record type found, corrupt storage?
      //return -2;
      break;
    }
  }
  gui_refresh=1;
  appctx.idx=0; appctx.top=0;
  listseeds_mode=1;
}

// todo/fixme crashes if there's no seeds in the storage
int _listseeds(void) {
  extern unsigned char outbuf[crypto_secretbox_NONCEBYTES+crypto_secretbox_ZEROBYTES+BUF_SIZE];
  uint8_t **items = (uint8_t**) bufs[0].start;
  size_t item_len=0;
  unsigned int ptr = FLASH_BASE;
  unsigned char *outptr = outbuf, nlen;
  unsigned char nonce[crypto_secretbox_NONCEBYTES];
  unsigned char cipher[crypto_secretbox_KEYBYTES+crypto_secretbox_ZEROBYTES];
  unsigned char plain[crypto_secretbox_KEYBYTES+crypto_secretbox_ZEROBYTES];
  UserRecord *userdata = get_userrec();

  if(userdata==0) {
    oled_clear();
    oled_print(0,0,"uninitialized",Font_8x8);
    return 1;
  }

  while(ptr < FLASH_BASE + FLASH_SECTOR_SIZE && *((unsigned char*)ptr) != EMPTY ) {
    if(*((unsigned char*)ptr) != SEED) { // only seeds
        goto endloop;
    }

    // try to unmask name
    nlen = get_peer(outptr, (unsigned char*) ((SeedRecord*) ptr)->peerid);
    if(nlen==0 || nlen >= PEER_NAME_MAX) {
      // couldn't map peerid to name
      // what to do? write "unresolvable name"
      memcpy(outptr, "<corrupt>", 9);
      nlen=9;
    }
    outptr[nlen++] = 0; //terminate it
    // test if seed can be decrypted

    // pad ciphertext with extra 16 bytes
    memcpy(cipher+crypto_secretbox_BOXZEROBYTES,
           ((SeedRecord*) ptr)->mac,
           crypto_scalarmult_curve25519_BYTES+crypto_secretbox_MACBYTES);
    // why >>2 ?
    memset(cipher, 0, crypto_secretbox_BOXZEROBYTES>>2);
    // nonce
    crypto_generichash(nonce, crypto_secretbox_NONCEBYTES,                  // output
                       (unsigned char*) ((SeedRecord*) ptr)->peerid, STORAGE_ID_LEN<<1, // input
                       (unsigned char*) userdata->salt, USER_SALT_LEN);      // key
    // decrypt
    // todo/bug erase plain after usage, leaks keymaterial
    if(crypto_secretbox_open(plain,                 // ciphertext output
                             cipher,                // plaintext input
                             crypto_scalarmult_curve25519_BYTES+crypto_secretbox_ZEROBYTES, // plain length
                             nonce,                 // nonce
                             get_master_key())      // key
       == -1) {
      // rewind name of corrupt seed
      goto endloop;
    }
    items[item_len]=outptr;
    item_len++;
    outptr+=nlen;

  endloop:
    if( (ptr = next_rec(ptr)) == -1) {
      // invalid record type found, corrupt storage?
      //return -2;
      break;
    }
  }
  // todo/bug if storage area has no seeds, then it displays bullcrap :/
  return menu(&appctx, (const uint8_t**) items,item_len,listseeds_cb);
}

void test(void) {
  gui_refresh=1;
  appctx.idx=0; appctx.top=0;
  while(1) {
    if(listseeds_mode == 0)
      _listseeds();
    else {
      if(menu(&appctx, (const uint8_t**) seedmenuitems,sizeof(seedmenuitems),seedactions_cb) == 0) {
        memset(selected_seed,0,STORAGE_ID_LEN);
        listseeds_mode = 0;
        gui_refresh=1;
        appctx.idx=0; appctx.top=0;
      }
    }
  }
  rsp_detach();
  rsp_finish();
  while(1);
}
