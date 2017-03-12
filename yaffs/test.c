#include "pitchfork.h"
#include <string.h>
#include "yaffs_guts.h"

struct yaffs_dev* yaffs_StartUp(void);

unsigned char *ucHeap;

void vPortFree( void *pv );
void *pvPortMalloc( size_t xWantedSize );

void listdir(const char *dname) {
  yaffs_DIR *d;
  struct yaffs_dirent *de;

  d = yaffs_opendir(dname);
  if(!d) {
    while(1);
  } else {
    while((de = yaffs_readdir(d)) != NULL) {
      //sprintf(str,"%s/%s",dname,de->d_name);


      //printf("%s ino %d length %d mode %X ",
      //       de->d_name, (int)s.st_ino, (int)s.st_size, s.st_mode);
      //switch(s.st_mode & S_IFMT) {
      //case S_IFREG: printf("data file"); break;
      //case S_IFDIR: printf("directory"); break;
      //case S_IFLNK: printf("symlink -->");
      // if(yaffs_readlink(str,str,100) < 0)
      //   printf("no alias");
      // else
      //   printf("\"%s\"",str);
      // break;
      //default: printf("unknown"); break;
      //}

      //printf("\n");
    }
    yaffs_closedir(d);
  }
}

void test(void) {
  ucHeap = outbuf;
  memset(ucHeap, 0, 65536);
  //void* ptr1 = pvPortMalloc(1024);
  //void* ptr2 = pvPortMalloc(1024);
  //vPortFree(ptr2);
  //vPortFree(ptr1);
  yaffs_StartUp();

  if(yaffs_mount("/") < 0){
    while(1);
  }

  if(yaffs_mkdir("/test",0666) != 1) {
    while(1);
  }

  int fd = yaffs_open("/test/test",O_CREAT| O_RDWR, 0666);
  yaffs_lseek(fd,16,SEEK_SET);
  yaffs_write(fd,"asdf",4);
  yaffs_close(fd);
  listdir("/test");
  yaffs_rename("/test/test","/test/asdf");
  struct yaffs_stat s;
  yaffs_lstat("/test/asdf",&s);
  listdir("/test");

  fd = yaffs_open("/test/asdf",O_CREAT| O_RDWR, 0666);
  yaffs_lseek(fd,16,SEEK_SET);
  char outbuf[4];
  yaffs_read(fd,outbuf,4);
  yaffs_close(fd);

  yaffs_unlink("/test/test");
  listdir("/test");
  listdir("/");
  yaffs_rmdir("/test");
  listdir("/");
  return;
}
