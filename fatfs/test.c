#include "src/ff.h"
#include "usb.h"
#include "pitchfork.h"
#include "stm32f.h"
#include "randombytes_salsa20_random.h"

#define DIV85(number) ((unsigned int)(((unsigned long long)3233857729u * number) >> 32) >> 6)

static const char* base85 = {
   "0123456789"
   "abcdefghij"
   "klmnopqrst"
   "uvwxyzABCD"
   "EFGHIJKLMN"
   "OPQRSTUVWX"
   "YZ.-:+=^!/"
   "*?&<>()[]{"
   "}@%$#" };

void test(void) {
  FATFS fs;
  FIL fsrc;      // file objects
  UINT bw;         // File W count
  volatile FRESULT res;
  unsigned char key[crypto_secretbox_KEYBYTES];
  unsigned char ekid[EKID_LEN+EKID_NONCE_LEN];
  unsigned int v,v2,sptr;
  unsigned char fname[] = "2:/                                        \0"; // zero terminated 40 byte long z85 filename
  unsigned char peer[9] = "pitchfork";
  unsigned char* src = ekid;
  unsigned char* dst = fname+3;
  int i, size = BUF_SIZE;

  //while((f_mount(&fs, (const TCHAR*) fname, 0) == FR_NO_FILESYSTEM) && fname[0]++ < '4');
  res = f_mount(&fs, (const TCHAR*) fname, 0);

  // calculate ephemeral keyid into z85 filename
  sptr = get_peer_seed(key, peer, sizeof(peer));
  get_ekid(((unsigned char*) ((SeedRecord*) sptr)->keyid), ekid+EKID_LEN, ekid);

  for (;src < ekid+EKID_SIZE; src += 4, dst += 5) {
    // unpack big-endian frame
    v = (src[0] << 24) | (src[1] << 16) | (src[2] << 8) | src[3];

    v2 = DIV85(v); dst[4] = base85[v - v2 * 85]; v = v2;
    v2 = DIV85(v); dst[3] = base85[v - v2 * 85]; v = v2;
    v2 = DIV85(v); dst[2] = base85[v - v2 * 85]; v = v2;
    v2 = DIV85(v); dst[1] = base85[v - v2 * 85];
    dst[0] = base85[v2];
  }

  // zero out beginning of plaintext as demanded by nacl
  for(i=0;i<(crypto_secretbox_ZEROBYTES>>2);i++) ((unsigned int*) bufs[0].buf)[i]=0;
  // get nonce
  randombytes_salsa20_random_buf(outbuf, crypto_secretbox_NONCEBYTES);
  // encrypt (key is stored in beginning of params)
  crypto_secretbox(outbuf+crypto_secretbox_NONCEBYTES, bufs[0].buf, size+crypto_secretbox_ZEROBYTES, outbuf, key);
  // move nonce over boxzerobytes - so it's
  // concated to the ciphertext for sending
  for(i=(crypto_secretbox_NONCEBYTES>>2)-1;i>=0;i--)
    ((unsigned int*) outbuf)[(crypto_secretbox_BOXZEROBYTES>>2)+i] = ((unsigned int*) outbuf)[i];
  size+=crypto_secretbox_NONCEBYTES + crypto_secretbox_ZEROBYTES -crypto_secretbox_BOXZEROBYTES ; // add nonce+mac size to total size

  res = f_open(&fsrc,(const TCHAR *) fname, FA_OPEN_ALWAYS | FA_WRITE);
  res = f_lseek(&fsrc, f_size(&fsrc)); // seek to end
  res = f_write(&fsrc,(outbuf+crypto_secretbox_BOXZEROBYTES),size,(void *)&bw);
  res = f_close(&fsrc);
  (void)res; // shut up
}

