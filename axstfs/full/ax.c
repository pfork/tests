#include <stdio.h>
#include <string.h>
#include <sodium.h>
#include "stfs.h"
#include "axolotl.h"

// todo remove only for dumping image
#include <fcntl.h>
#include <unistd.h>

// todo remove in fw, replace with master.h get_master
static const uint8_t mk[32]={0};
// todo not needed in fw
void dump_info(Chunk blocks[NBLOCKS][CHUNKS_PER_BLOCK]);

Chunk blocks[NBLOCKS][CHUNKS_PER_BLOCK];
uint8_t eccdir[]="/keys/                                "; // 32 byte space at end for keyid
uint8_t prekeydir[]="/prekeys/                                "; // 32 byte space at end for keyid
uint8_t axolotldir[]="/axolotl/                                "; // 32 byte space at end for keyid

void stohex(uint8_t* d, uint8_t *s, uint32_t len) {
  if(d==NULL) return;
  if(s==NULL) return;

  uint32_t i, rc;
  for(i=0;i<len;i++) {
    rc=((*s)>>4)&0xF;
    if(rc>9) rc+=87; else rc+=0x30;
    *(d++)=rc;
    rc=(*s++)&0xF;
    if(rc>9) rc+=87; else rc+=0x30;
    *(d++)=rc;
  }
  *d=0;
}

#ifdef DEBUG_LEVEL
#define LOG(level, ...) if(DEBUG_LEVEL>=level) fprintf(stderr, ##__VA_ARGS__)
#else
#define LOG(level, ...)
#endif // DEBUG_LEVEL

// closes fd
// plain needs to be crypto_secretbox_ZEROBYTES padded, len not!
static int ax_store(int fd, uint8_t *plain, uint32_t len) {
  // nonce for encryption, for efficiency stored at beginning of output buffer
  uint8_t out[crypto_secretbox_NONCEBYTES+len+crypto_secretbox_MACBYTES];
  randombytes_buf((void *) out, crypto_secretbox_NONCEBYTES);
  // padded output buffer
  uint8_t outtmp[crypto_secretbox_ZEROBYTES+len];
  // zeroed out
  memset(plain,0, crypto_secretbox_ZEROBYTES);
  crypto_secretbox(outtmp,                         // ciphertext output
                   (uint8_t*) plain,               // plaintext input
                   len+crypto_secretbox_ZEROBYTES, // plain length
                   out,                            // nonce
                   mk);                            // key
                   //get_master_key());            // key

  // clear plaintext seed in RAM
  memset(plain,0, len+crypto_secretbox_ZEROBYTES);
  memcpy(out+crypto_secretbox_NONCEBYTES, outtmp+crypto_secretbox_BOXZEROBYTES, len+crypto_secretbox_MACBYTES);
  int ret;
  if((ret=stfs_write(fd, out, crypto_secretbox_NONCEBYTES+len+crypto_secretbox_MACBYTES, blocks)) !=
                              crypto_secretbox_NONCEBYTES+len+crypto_secretbox_MACBYTES) {
    LOG(1,"[x] failed to write whole file, only %d written, err: %d\n", ret, stfs_geterrno());
    stfs_close(fd,blocks);
    stfs_unlink(blocks, eccdir);
    return -1;
  }
  if(stfs_close(fd,blocks)==-1) {
    LOG(1,"[x] failed to close keyfile, err: %d\n", stfs_geterrno());
    stfs_unlink(blocks, eccdir);
    return -1;
  }

  return 0;
}

static int ax_load(uint8_t *fname, uint8_t *buf, uint32_t len) {
  int fd;
  if((fd=stfs_open(fname, 0, blocks))==-1) {
    LOG(1,"[x] failed to open %s, err: %d\n", fname, stfs_geterrno());
    return -1;
  }

  unsigned char cipher[len + crypto_secretbox_ZEROBYTES];
  memset(cipher,0, crypto_secretbox_BOXZEROBYTES);

  uint8_t nonce[crypto_secretbox_NONCEBYTES];
  if(stfs_read(fd,nonce,crypto_secretbox_NONCEBYTES,blocks)!=crypto_secretbox_NONCEBYTES) {
    LOG(1,"[x] failed reading nonce from file '%s' err: %d\n", fname, stfs_geterrno());
    return -1;
    if(stfs_close(fd,blocks)==-1) {
      LOG(1,"[x] failed to close file, err: %d\n", stfs_geterrno());
      return -1;
    }
  }

  int size=stfs_read(fd,cipher+crypto_secretbox_BOXZEROBYTES,len + crypto_secretbox_MACBYTES,blocks);
  if(stfs_close(fd,blocks)==-1) {
    LOG(1,"[x] failed to close file, err: %d\n", stfs_geterrno());
    return -1;
  }
  if(size!=len + crypto_secretbox_MACBYTES) {
    LOG(1,"[x] file '%s' is to short: %d, expected: %d\n", fname, size, len + crypto_secretbox_MACBYTES);
    return -1;
  }
  unsigned char plain[len + crypto_secretbox_ZEROBYTES];
  // decrypt
  if(crypto_secretbox_open(plain, cipher, len+crypto_secretbox_ZEROBYTES, nonce, mk /*get_master_key()*/) == -1) {
    return -1;
  }
  memcpy(buf, plain+crypto_secretbox_ZEROBYTES,len);
  memset(plain,0,sizeof(plain));

  return 0;
}

int open_uniq(uint8_t *path, int off) {
  //keyid
  uint8_t keyid[16];
  randombytes_buf(keyid, sizeof(keyid));
  stohex(path+off,keyid, sizeof(keyid));
  int fd, retries=10;
  while((fd=stfs_open(path, 64, blocks))==-1 && retries-->0) {
    // make sure it is new
    randombytes_buf(keyid, sizeof(keyid));
    stohex(path+off,keyid, sizeof(keyid));
  }
  LOG(3,"[i] opened '%s'\n", path);
  if(fd==-1) {
    LOG(1, "[x] failed to create unique file, err: %d\n", stfs_geterrno());
    return -1;
  }
  return fd;
}

uint8_t* ax_store_keypair(const Axolotl_KeyPair *keypair, const char *name) {
  int len=strlen(name);
  uint8_t plain[crypto_secretbox_ZEROBYTES+crypto_scalarmult_curve25519_BYTES+len];
  memcpy(plain+crypto_secretbox_ZEROBYTES, keypair->sk, crypto_scalarmult_curve25519_BYTES);
  memcpy(plain+crypto_secretbox_ZEROBYTES+crypto_scalarmult_curve25519_BYTES, name, len);
  len+=crypto_scalarmult_curve25519_BYTES;

  int fd=open_uniq(eccdir, 6);
  if(fd==-1) {
    LOG(1, "[x] failed to create unique file, err: %d\n", stfs_geterrno());
    return NULL;
  }

  if(ax_store(fd, plain, len)!=0) {
      LOG(1, "[x] failed to store key '%s'\n", axolotldir);
      return NULL;
  }

  return eccdir;
}

int ax_load_keypair(Axolotl_KeyPair *keypair, uint8_t* fname, uint8_t *name) {
  // todo refactor currently cannot use ax_load, since name is variable length :/
  int fd;
  if((fd=stfs_open(fname, 0, blocks))==-1) {
    LOG(1,"[x] failed to open %s, err: %d\n", fname, stfs_geterrno());
    return -1;
  }

  uint8_t nonce[crypto_secretbox_NONCEBYTES];
  if(stfs_read(fd,nonce,crypto_secretbox_NONCEBYTES,blocks)!=crypto_secretbox_NONCEBYTES) {
    LOG(1,"[x] failed reading nonce from keyfile '%s' err: %d\n", fname, stfs_geterrno());
    stfs_close(fd, blocks);
    return -1;
  }

  uint8_t cipher[crypto_secretbox_ZEROBYTES+1024];
  memset(cipher,0, crypto_secretbox_BOXZEROBYTES);
  int len=stfs_read(fd,cipher+crypto_secretbox_BOXZEROBYTES,1024,blocks);
  if(stfs_close(fd,blocks)==-1) {
    LOG(1,"[x] failed to close keyfile, err: %d\n", stfs_geterrno());
    return -1;
  }
  int size = len - crypto_secretbox_MACBYTES;
  if(size<1+crypto_scalarmult_curve25519_BYTES) {
    LOG(1,"[x] keyfile '%s' is to short: %d\n", fname, len);
    return -1;
  }

  unsigned char plain[size + crypto_secretbox_ZEROBYTES];
  // decrypt
  if(crypto_secretbox_open(plain, cipher, size+crypto_secretbox_ZEROBYTES, nonce, mk /*get_master_key()*/) == -1) {
    LOG(1,"[x] failed do decrypt keyfile '%s'\n", fname);
    return -1;
  }
  memcpy(keypair->sk, plain+crypto_secretbox_ZEROBYTES, crypto_scalarmult_curve25519_BYTES);
  if(name!=NULL) {
    memcpy(name, plain+crypto_secretbox_ZEROBYTES+crypto_scalarmult_curve25519_BYTES, size-crypto_scalarmult_curve25519_BYTES);
    name[size-crypto_scalarmult_curve25519_BYTES]=0;
  }

  memset(plain,0,sizeof(plain));
  crypto_scalarmult_curve25519_base(keypair->pk, keypair->sk);
  return 0;
}

uint8_t* ax_store_prekey(const Axolotl_PreKey *prekey, uint8_t *ltkeyid) {
  const int len=crypto_scalarmult_curve25519_BYTES*2+39; // '39=len('/keys/<keyid>')
  uint8_t plain[crypto_secretbox_ZEROBYTES+len];
  memcpy(plain+crypto_secretbox_ZEROBYTES,
         prekey->DHRs,
         crypto_scalarmult_curve25519_BYTES);
  memcpy(plain+crypto_secretbox_ZEROBYTES+crypto_scalarmult_curve25519_BYTES,
         prekey->ephemeralkey,
         crypto_scalarmult_curve25519_BYTES);
  // todo slim down: ltkeyid in prekey
  // storing like this the ltkeyid is overkill! we could do it in less than 1/2
  memcpy(plain+crypto_secretbox_ZEROBYTES+crypto_scalarmult_curve25519_BYTES*2,
         ltkeyid, 39);

  int fd=open_uniq(prekeydir, 9);
  if(fd==-1) {
    LOG(1, "[x] failed to create unique file, err: %d\n", stfs_geterrno());
    return NULL;
  }

  if(ax_store(fd, plain, len)!=0) {
      LOG(1, "[x] failed to store ctx '%s'\n", axolotldir);
      return NULL;
  }

  return prekeydir;
}

int ax_load_prekey(Axolotl_PreKey *prekey, uint8_t *fname) {
  const int len=crypto_scalarmult_curve25519_BYTES*2+39; // '39=len('/keys/<keyid>')
  uint8_t buf[len];
  if(ax_load(fname, buf, len)!=0) {
    LOG(1,"[x] failed to load prekey %s, err: %d\n", fname, stfs_geterrno());
    return -1;
  }

  memcpy(prekey->DHRs, buf,crypto_scalarmult_curve25519_BYTES);
  memcpy(prekey->ephemeralkey, buf+crypto_scalarmult_curve25519_BYTES,crypto_scalarmult_curve25519_BYTES);
  // get keypair for lt_key
  Axolotl_KeyPair keypair;
  uint8_t name[32+7];
  if(ax_load_keypair(&keypair, buf+crypto_scalarmult_curve25519_BYTES*2, name)!=0) {
    LOG(1,"[x] failed to load lt keypair '%s' for prekey '%s'\n", buf+crypto_scalarmult_curve25519_BYTES*2, fname);
    return -1;
  }
  memset(buf,0,len);
  crypto_scalarmult_curve25519_base(prekey->identitykey, keypair.sk);
  memset(&keypair,0,sizeof(keypair));
  return 0;
}

int ax_del_prekey(uint8_t* fname) {
  return stfs_unlink(blocks, fname);
}

uint8_t* ax_store_ctx(const Axolotl_ctx *ctx, uint8_t *fname) {
  // todo optimize slimming down.
  uint8_t plain[crypto_secretbox_ZEROBYTES+sizeof(Axolotl_ctx)];
  memcpy(plain+crypto_secretbox_ZEROBYTES, ctx, sizeof(Axolotl_ctx));

  int fd;
  if(fname!=NULL) {
    if((fd=stfs_open(fname, 0, blocks))==-1) {
      LOG(1, "[x] failed to re-open ctx '%s'\n", fname);
      return NULL;
    }
  } else {
    //new keyid
    stohex(axolotldir+9, (uint8_t*) ctx->eph.pk, sizeof(ctx->eph.pk)/2);
    if((fd=stfs_open(axolotldir, 0, blocks))==-1) {
      LOG(1, "[i] failed to open ctx '%s', trying to create\n", axolotldir);
      if((fd=stfs_open(axolotldir, 64, blocks))==-1) {
        LOG(1, "[x] failed to create ctx '%s'\n", axolotldir);
        return NULL;
      }
    }
    LOG(3,"[i] opened '%s'\n", axolotldir);
  }
  if(ax_store(fd, plain, sizeof(Axolotl_ctx))!=0) {
      LOG(1, "[x] failed to store ctx '%s'\n", axolotldir);
      return NULL;
  }

  return axolotldir;
}

int ax_load_ctx(Axolotl_ctx *ctx, uint8_t *fname) {
  // todo optimize slimming down.
  if(ax_load(fname, (uint8_t*) ctx, sizeof(Axolotl_ctx))!=0) {
    LOG(1,"[x] failed to load ctx %s, err: %d\n", fname, stfs_geterrno());
    return -1;
  }
  return 0;
}

int ax_send(uint8_t *path, uint8_t *cipher, uint32_t *clen, const uint8_t *plain, const uint32_t plen) {
  Axolotl_ctx ctx;
  if(ax_load_ctx(&ctx, path)!=0) {
    fprintf(stderr, "[x] failed to load ctx for '%s'\n", path);
    return -1;
  }
  axolotl_box(&ctx, cipher, clen, plain, plen);
  if(ax_store_ctx(&ctx, path)==NULL) {
    // fail to store ctx
    fprintf(stderr, "[x] failed to store 2nd ctx for alice\n");
    return -1;
  }
  return 0;
}

int ax_recv(uint8_t *path, uint8_t *plain, uint32_t *plen, const uint8_t *cipher, const uint32_t clen) {
  Axolotl_ctx ctx;
  if(ax_load_ctx(&ctx, path)!=0) {
    fprintf(stderr, "[x] failed to load ctx for '%s'\n", path);
    return -1;
  }
  if(axolotl_box_open(&ctx, plain, plen, cipher, clen)!=0) {
    // fail
    fprintf(stderr, "[x] failed to read message for '%s'\n", path);
    return -1;
  }
  if(ax_store_ctx(&ctx, path)==NULL) {
    // fail to store ctx
    fprintf(stderr, "[x] failed to store ctx for '%s'\n", path);
    return -1;
  }
  return 0;
}

int main(void) {
  memset(blocks,0xff,sizeof(blocks));
  if(stfs_init(blocks)==-1) {
    return 1;
  };

  eccdir[5]=0;
  stfs_mkdir(blocks, eccdir);
  eccdir[5]='/';

  prekeydir[8]=0;
  stfs_mkdir(blocks, prekeydir);
  prekeydir[8]='/';

  axolotldir[8]=0;
  stfs_mkdir(blocks, axolotldir);
  axolotldir[8]='/';

  Axolotl_KeyPair alice_id, bob_id;
  Axolotl_PreKey alice_prekey, bob_prekey;
  Axolotl_ctx alice_ctx, bob_ctx;
  uint8_t alice_kid[7+32], bob_kid[7+32];

  // init long-term identity keys
  uint8_t *res;
  axolotl_genid(&alice_id);
  if((res=ax_store_keypair(&alice_id, "alice"))==NULL) {
    // fail to store key
    fprintf(stderr, "[x] failed to store key for alice\n");
    goto exit;
    //return 1;
  }
  memcpy(alice_kid, res, sizeof(alice_kid));

  axolotl_genid(&bob_id);
  if((res=ax_store_keypair(&bob_id, "bob"))==NULL) {
    // fail to store key
    fprintf(stderr, "[x] failed to store key for bob\n");
    goto exit;
    //return 1;
  }
  memcpy(bob_kid, res, sizeof(bob_kid));

  // load long term keys to generate prekeys
  Axolotl_KeyPair alice_id1, bob_id1;
  uint8_t alice_name[33], bob_name[33];
  LOG(3,"[i] loading key at '%s'\n", alice_kid);
  if(ax_load_keypair(&alice_id1, alice_kid, alice_name)!=0) {
    fprintf(stderr, "[x] failed to load key for alice\n");
    goto exit;
    //return 1;
  }
  fprintf(stderr, "[i] load key for %s\n", alice_name);
  LOG(3,"[i] loading key at '%s'\n", bob_kid);
  if(ax_load_keypair(&bob_id1, bob_kid, bob_name)!=0) {
    fprintf(stderr, "[x] failed to load key for bob\n");
    goto exit;
    //return 1;
  }
  fprintf(stderr, "[i] load key for %s\n", bob_name);

  uint8_t alice_pkid[10+32], bob_pkid[10+32];
  int i;
  for(i=0;i<100;i++) {
    axolotl_prekey(&alice_prekey, &alice_ctx, &alice_id1);
    if((res=ax_store_prekey(&alice_prekey, alice_kid))==NULL) {
      // fail to store key
      fprintf(stderr, "[x] failed to store prekey for alice\n");
      goto exit;
      //return 1;
    }
    memcpy(alice_pkid, res, sizeof(alice_pkid));
  }

  for(i=0;i<100;i++) {
    axolotl_prekey(&bob_prekey, &bob_ctx, &bob_id1);
    if((res=ax_store_prekey(&bob_prekey, bob_kid))==NULL) {
      // fail to store key
      fprintf(stderr, "[x] failed to store prekey for bob\n");
      goto exit;
      //return 1;
    }
    memcpy(bob_pkid, res, sizeof(bob_pkid));
  }

  // both derive the ctx from their exchanged prekey msg
  Axolotl_PreKey alice_prekey1, bob_prekey1;
  if(ax_load_prekey(&alice_prekey1, alice_pkid)!=0) {
    fprintf(stderr, "[x] failed to load prekey for alice\n");
    goto exit;
  }
  if(ax_load_prekey(&bob_prekey1, bob_pkid)!=0) {
    fprintf(stderr, "[x] failed to load prekey for bob\n");
    goto exit;
  }

  axolotl_handshake(&alice_ctx, &bob_prekey1);
  uint8_t alice_cid[10+32], bob_cid[10+32];
  if((res=ax_store_ctx(&alice_ctx, NULL))==NULL) {
    // fail to store ctx
    fprintf(stderr, "[x] failed to store ctx for alice\n");
    goto exit;
    //return 1;
  }
  memcpy(alice_cid, res, sizeof(alice_cid));
  ax_del_prekey(alice_pkid);

  axolotl_handshake(&bob_ctx, &alice_prekey1);
  if((res=ax_store_ctx(&bob_ctx, NULL))==NULL) {
    // fail to store ctx
    fprintf(stderr, "[x] failed to store ctx for bob\n");
    goto exit;
    //return 1;
  }
  memcpy(bob_cid, res, sizeof(bob_cid));
  ax_del_prekey(bob_pkid);

  Axolotl_ctx alice_ctx1, bob_ctx1;
  uint8_t out[4096], out2[4096];
  uint32_t outlen,outlen2;

  if(ax_load_ctx(&alice_ctx1, alice_cid)!=0) {
    fprintf(stderr, "[x] failed to load ctx for alice\n");
    goto exit;
  }
  axolotl_box(&alice_ctx1, out, &outlen, (const uint8_t *) "howdy", 6);
  if(ax_store_ctx(&alice_ctx1, alice_cid)==NULL) {
    // fail to store ctx
    fprintf(stderr, "[x] failed to store 2nd ctx for alice\n");
    goto exit;
    //return 1;
  }

  if(ax_load_ctx(&bob_ctx1, bob_cid)!=0) {
    fprintf(stderr, "[x] failed to load ctx for bob\n");
    goto exit;
  }
  if(axolotl_box_open(&bob_ctx, out2, &outlen2, out, outlen)!=0) {
    // fail
    fprintf(stderr, "[x] failed to read message for bob\n");
    goto exit;
  }
  if(ax_store_ctx(&bob_ctx1, bob_cid)==NULL) {
    // fail to store ctx
    fprintf(stderr, "[x] failed to store 2nd ctx for bob\n");
    goto exit;
    //return 1;
  }
  fprintf(stderr, "[ ] message for bob: '%s'\n", out2);

  ax_send(alice_cid, out, &outlen, (const uint8_t *) "2nd howdy", 10);
  if(ax_recv(bob_cid, out2, &outlen2, out, outlen)!=0) {
    fprintf(stderr, "[x] failed to read message for bob\n");
    goto exit;
  }
  fprintf(stderr, "[ ] message for bob: '%s'\n", out2);

  ax_send(bob_cid, out, &outlen, (const uint8_t *) "re", 3);
  if(ax_recv(alice_cid, out2, &outlen2, out, outlen)!=0) {
    fprintf(stderr, "[x] failed to read message for alice\n");
    goto exit;
  }
  fprintf(stderr, "[ ] message for alice: '%s'\n", out2);

  ax_send(alice_cid, out, &outlen, (const uint8_t *) "rerere", 7);
  if(ax_recv(bob_cid, out2, &outlen2, out, outlen)!=0) {
    fprintf(stderr, "[x] failed to read message for bob\n");
    goto exit;
  }
  fprintf(stderr, "[ ] message for bob: '%s'\n", out2);

  ax_send(alice_cid, out, &outlen, (const uint8_t *) "2nd rerere", 11);
  if(ax_recv(bob_cid, out2, &outlen2, out, outlen)!=0) {
    fprintf(stderr, "[x] failed to read message for bob\n");
    goto exit;
  }
  fprintf(stderr, "[ ] message for bob: '%s'\n", out2);

  ax_send(bob_cid, out, &outlen, (const uint8_t *) "rerere", 7);
  if(ax_recv(alice_cid, out2, &outlen2, out, outlen)!=0) {
    fprintf(stderr, "[x] failed to read message for alice\n");
    goto exit;
  }
  fprintf(stderr, "[ ] message for alice: '%s'\n", out2);

  // test 7
  // some out of order sending
  /*
    msgx1 = ctx2.send("aaaaa")
    msg1 = ctx1.send("00000")
    msg2 = ctx1.send("11111")
    msgx2 = ctx2.send("bbbbb")
    msgx3 = ctx2.send("ccccc")
    msg3 = ctx1.send("22222")
    msg4 = ctx1.send("33333")
    msgx4 = ctx2.send("ddddd")
    assert(ctx2.recv(msg2) == '11111')
   */
  uint8_t msg[5][axolotl_box_BYTES+6];
  uint8_t msgx[5][axolotl_box_BYTES+6];
  for(i=0;i<10000;i++) {
    ax_send(alice_cid, msg[0], &outlen, (const uint8_t *) "00000", 6);
    ax_send(alice_cid, msg[1], &outlen, (const uint8_t *) "11111", 6);
    ax_send(alice_cid, msg[2], &outlen, (const uint8_t *) "22222", 6);
    ax_send(alice_cid, msg[3], &outlen, (const uint8_t *) "33333", 6);
    ax_send(bob_cid, msgx[0], &outlen, (const uint8_t *) "aaaaa", 6);
    ax_send(bob_cid, msgx[1], &outlen, (const uint8_t *) "bbbbb", 6);
    ax_send(bob_cid, msgx[2], &outlen, (const uint8_t *) "ccccc", 6);
    ax_send(bob_cid, msgx[3], &outlen, (const uint8_t *) "ddddd", 6);

    if(ax_recv(bob_cid, out2, &outlen2, msg[1], axolotl_box_BYTES+6)!=0) {
      fprintf(stderr, "[x] failed to read message for bob\n");
      goto exit;
    }
    LOG(3, "[ ] message for bob: '%s'\n", out2);

    ax_send(alice_cid, msg[4], &outlen, (const uint8_t *) "44444", 6);

    /* printf("bob receives 44444\n"); */
    if(ax_recv(bob_cid, out2, &outlen2, msg[4], axolotl_box_BYTES+6)!=0) {
      fprintf(stderr, "[x] failed to read message for bob\n");
      goto exit;
    }
    LOG(3, "[ ] message for bob: '%s'\n", out2);

    ax_send(bob_cid, msgx[4], &outlen, (const uint8_t *) "eeeee", 6);

    /* printf("alice receives aaaaa\n"); */
    if(ax_recv(alice_cid, out2, &outlen2, msgx[0], axolotl_box_BYTES+6)!=0) {
      fprintf(stderr, "[x] failed to read message for alice\n");
      goto exit;
    }
    LOG(3, "[ ] message for alice: '%s'\n", out2);

    /* printf("alice receives ccccc\n"); */
    if(ax_recv(alice_cid, out2, &outlen2, msgx[2], axolotl_box_BYTES+6)!=0) {
      fprintf(stderr, "[x] failed to read message for alice\n");
      goto exit;
    }
    LOG(3, "[ ] message for alice: '%s'\n", out2);

    /* printf("alice receives eeeee\n"); */
    if(ax_recv(alice_cid, out2, &outlen2, msgx[4], axolotl_box_BYTES+6)!=0) {
      fprintf(stderr, "[x] failed to read message for alice\n");
      goto exit;
    }
    LOG(3, "[ ] message for alice: '%s'\n", out2);

    /* printf("bob receives 33333\n"); */
    if(ax_recv(bob_cid, out2, &outlen2, msg[3], axolotl_box_BYTES+6)!=0) {
      fprintf(stderr, "[x] failed to read message for bob\n");
      goto exit;
    }
    LOG(3, "[ ] message for bob: '%s'\n", out2);

    /* printf("alice receives ddddd\n"); */
    if(ax_recv(alice_cid, out2, &outlen2, msgx[3], axolotl_box_BYTES+6)!=0) {
      fprintf(stderr, "[x] failed to read message for alice\n");
      goto exit;
    }
    LOG(3, "[ ] message for alice: '%s'\n", out2);

    /* printf("bob receives 22222\n"); */
    if(ax_recv(bob_cid, out2, &outlen2, msg[2], axolotl_box_BYTES+6)!=0) {
      fprintf(stderr, "[x] failed to read message for bob\n");
      goto exit;
    }
    LOG(3, "[ ] message for bob: '%s'\n", out2);

    /* printf("alice receives bbbbb\n"); */
    if(ax_recv(alice_cid, out2, &outlen2, msgx[1], axolotl_box_BYTES+6)!=0) {
      fprintf(stderr, "[x] failed to read message for alice\n");
      goto exit;
    }
    LOG(3, "[ ] message for alice: '%s'\n", out2);

    /* printf("bob receives 00000\n"); */
    if(ax_recv(bob_cid, out2, &outlen2, msg[0], axolotl_box_BYTES+6)!=0) {
      fprintf(stderr, "[x] failed to read message for bob\n");
      goto exit;
    }
    LOG(3, "[ ] message for bob: '%s'\n", out2);
  }

 exit:
  dump_info(blocks);
  int fd;
  fd=open("test.img", O_RDWR | O_CREAT | O_TRUNC, 0666 );
  fprintf(stderr, "[i] dumping fs to fd %d\n", fd);
  write(fd,blocks, sizeof(blocks));
  close(fd);

  return 0;
}
