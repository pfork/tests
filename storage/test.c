#include "storage.h"
#include <string.h>
#include "crypto_scalarmult_curve25519.h"
#include "../rsp.h"
#include "pitchfork.h"

// plaintext lookup key(id) by peer name
// ciphertext with ephemeral keyid
// can lookup with key the name of the peer

void test(void) {
  unsigned char seed1[crypto_scalarmult_curve25519_BYTES];
  unsigned char peer1[]="peer1";
  unsigned char seed2[crypto_scalarmult_curve25519_BYTES];
  unsigned char peer2[]="peer2";
  unsigned char seed1x[crypto_scalarmult_curve25519_BYTES];
  unsigned int ptr;
  unsigned char peerid[STORAGE_ID_LEN];

  memset((void*) FLASH_BASE, 0xff, 1024);

  // create owner
  UserRecord *owner = init_user((unsigned char*) "stf", 3);
  (void) owner;

  // create peer1
  ptr = (int) store_seed(seed1, peer1, sizeof(peer1)-1);
  //rsp_dump((unsigned char *) ptr, sizeof(SeedRecord));

  // create peer2
  ptr = (int) store_seed(seed2, peer2, sizeof(peer2)-1);
  //rsp_dump((unsigned char *) ptr, sizeof(SeedRecord));

  // del peer1
  topeerid(peer1, sizeof(peer1)-1, peerid);
  del_seed(peerid, 0);

  // re-create peer1
  ptr = (int) store_seed(seed1, peer1, sizeof(peer1)-1);
  //rsp_dump((unsigned char *) ptr, sizeof(SeedRecord));

  // get seed for peer1
  topeerid(peer1, sizeof(peer1)-1, peerid);
  ptr = get_seed(seed1x, peerid, 0, 0);
  //rsp_dump((unsigned char *) ptr, sizeof(SeedRecord));

  // get seed for peer2
  topeerid(peer2, sizeof(peer2)-1, peerid);
  ptr = get_seed(seed1x, peerid, 0, 0);
  //rsp_dump((unsigned char *) ptr, sizeof(SeedRecord));

  // del peer2
  del_seed(0, (unsigned char*) ((SeedRecord*) ptr)->keyid);

  // re-create peer2
  ptr = (int) store_seed(seed2, peer2, sizeof(peer2)-1);
  //rsp_dump((unsigned char *) ptr, sizeof(SeedRecord));

  //rsp_dump((unsigned char *) FLASH_BASE, 512);

  ptr = FLASH_BASE;
  unsigned char *outptr = outbuf, nlen;
  memset(outbuf,0,512);

  while(ptr < FLASH_BASE + FLASH_SECTOR_SIZE && *((unsigned char*)ptr) != EMPTY ) {
    switch(*((unsigned char*) ptr)) {
    case SEED: {
      *(outptr++) ='s';
      *(outptr++) =' ';
      nlen = get_peer(outptr, (unsigned char*) ((SeedRecord*) ptr)->peerid);
      if(nlen==0 || nlen >= PEER_NAME_MAX) {
        // couldn't map peerid to name
        *(outptr-2)='u';
        *(outptr++) ='\n';
        break;
      }
      outptr+=nlen; //+1;
      *(outptr++) ='\n';
      break;
    }
    case PEERMAP: {
      *(outptr++) ='p';
      *(outptr++) =' ';
      nlen = get_peer(outptr, (unsigned char*) ((MapRecord*) ptr)->peerid);
      if(nlen==0 || nlen >= PEER_NAME_MAX) {
        // couldn't map peerid to name
        memcpy(outptr, "unknown", 7);
        outptr+=7;
        *(outptr++) ='\n';
        break;
      }
      outptr+=nlen; //+1;
      *(outptr++) ='\n';
      break;
    }
    case USERDATA: {
      *(outptr++) ='u';
      *(outptr++) =' ';
      memcpy(outptr, ((UserRecord*) ptr)->name, ((UserRecord*) ptr)->len- USERDATA_HEADER_LEN);
      outptr+=((UserRecord*) ptr)->len- USERDATA_HEADER_LEN;
      *(outptr++) ='\n';
      break;
    }
    case DELETED_SEED: {
      *(outptr++) ='S';
      *(outptr++) ='\n';
      break;
    }
    case DELETED_PEERMAP: {
      *(outptr++) ='P';
      *(outptr++) ='\n';
      break;
    }
    case DELETED_USERDATA: {
      *(outptr++) ='U';
      *(outptr++) ='\n';
      break;
    }
    case EMPTY: {
      *(outptr++) ='$';
      *(outptr++) ='\n';
      break;
    }
    }
    if( (ptr = next_rec(ptr)) == -1) {
      // invalid record type found, corrupt storage?
      //return -2;
      break;
    }
  }

  rsp_dump(outbuf, outptr - outbuf);

  rsp_detach();
  rsp_finish();
  while(1);
}
