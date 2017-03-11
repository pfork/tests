#include <stdint.h>
#include <unistd.h>
#include <string.h>

/* compile with
arm-none-eabi-gcc -mno-unaligned-access -g -Wall -Werror -Os -mfix-cortex-m3-ldrd -msoft-float -mthumb -Wno-strict-aliasing -fomit-frame-pointer -mcpu=cortex-m3 -fno-common -Tmemmap -nostartfiles -Wl,--gc-sections -Wl,-z,relro -o test.elf test.c

then in `arm-none-eabi-gdb test.elf` and attached via JTAG/SWI:
> load
> nexti # a couple times until in ?? signal handler
*/

static inline void __ISB()                      { asm volatile ("isb"); }
static inline void __DSB()                      { asm volatile ("dsb"); }

typedef struct {
  volatile const unsigned int CPUID;         /*!< Offset: 0x00  CPU ID Base Register                                  */
  volatile unsigned int ICSR;                /*!< Offset: 0x04  Interrupt Control State Register                      */
  volatile unsigned int VTOR;                /*!< Offset: 0x08  Vector Table Offset Register                          */
  volatile unsigned int AIRCR;               /*!< Offset: 0x0C  Application Interrupt / Reset Control Register        */
  volatile unsigned int SCR;                 /*!< Offset: 0x10  System Control Register                               */
  volatile unsigned int CCR;                 /*!< Offset: 0x14  Configuration Control Register                        */
  volatile unsigned char SHP[12];            /*!< Offset: 0x18  System Handlers Priority Registers (4-7, 8-11, 12-15) */
  volatile unsigned int SHCSR;               /*!< Offset: 0x24  System Handler Control and State Register             */
  volatile unsigned int CFSR;                /*!< Offset: 0x28  Configurable Fault Status Register                    */
  volatile unsigned int HFSR;                /*!< Offset: 0x2C  Hard Fault Status Register                            */
  volatile unsigned int DFSR;                /*!< Offset: 0x30  Debug Fault Status Register                           */
  volatile unsigned int MMFAR;               /*!< Offset: 0x34  Mem Manage Address Register                           */
  volatile unsigned int BFAR;                /*!< Offset: 0x38  Bus Fault Address Register                            */
  volatile unsigned int AFSR;                /*!< Offset: 0x3C  Auxiliary Fault Status Register                       */
  volatile const  unsigned int PFR[2];       /*!< Offset: 0x40  Processor Feature Register                            */
  volatile const  unsigned int DFR;          /*!< Offset: 0x48  Debug Feature Register                                */
  volatile const  unsigned int ADR;          /*!< Offset: 0x4C  Auxiliary Feature Register                            */
  volatile const  unsigned int MMFR[4];      /*!< Offset: 0x50  Memory Model Feature Register                         */
  volatile const  unsigned int ISAR[5];      /*!< Offset: 0x60  ISA Feature Register                                  */
} SCB_Type;

#define PPBI_BASE                       0xE0000000
#define SCS_BASE                        (PPBI_BASE + 0xE000)
#define SCB_BASE            (SCS_BASE +  0x0D00)                      /*!< System Control Block Base Address */
#define SCB             ((SCB_Type *) SCB_BASE)

#define SCB_SHCSR_MEMFAULTENA_Pos          16                                             /*!< SCB SHCSR: MEMFAULTENA Position */
#define SCB_SHCSR_MEMFAULTENA_Msk          (1ul << SCB_SHCSR_MEMFAULTENA_Pos)             /*!< SCB SHCSR: MEMFAULTENA Mask */

typedef struct {
  volatile const  uint32_t TYPE;                  /*!< Offset: 0x00  MPU Type Register                              */
  volatile uint32_t CTRL;                         /*!< Offset: 0x04  MPU Control Register                           */
  volatile uint32_t RNR;                          /*!< Offset: 0x08  MPU Region RNRber Register                     */
  volatile uint32_t RBAR;                         /*!< Offset: 0x0C  MPU Region Base Address Register               */
  volatile uint32_t RASR;                         /*!< Offset: 0x10  MPU Region Attribute and Size Register         */
  volatile uint32_t RBAR_A1;                      /*!< Offset: 0x14  MPU Alias 1 Region Base Address Register       */
  volatile uint32_t RASR_A1;                      /*!< Offset: 0x18  MPU Alias 1 Region Attribute and Size Register */
  volatile uint32_t RBAR_A2;                      /*!< Offset: 0x1C  MPU Alias 2 Region Base Address Register       */
  volatile uint32_t RASR_A2;                      /*!< Offset: 0x20  MPU Alias 2 Region Attribute and Size Register */
  volatile uint32_t RBAR_A3;                      /*!< Offset: 0x24  MPU Alias 3 Region Base Address Register       */
  volatile uint32_t RASR_A3;                      /*!< Offset: 0x28  MPU Alias 3 Region Attribute and Size Register */
} MPU_Regs;

#define MPU_BASE          (SCS_BASE +  0x0D90)
#define MPU               ((MPU_Regs*) MPU_BASE)

#define MPU_CTRL_PRIVDEFENA_Pos             2                                             /*!< MPU CTRL: PRIVDEFENA Position */
#define MPU_CTRL_PRIVDEFENA_Msk            (1ul << MPU_CTRL_PRIVDEFENA_Pos)               /*!< MPU CTRL: PRIVDEFENA Mask */

#define MPU_CTRL_HFNMIENA_Pos               1                                             /*!< MPU CTRL: HFNMIENA Position */
#define MPU_CTRL_HFNMIENA_Msk              (1ul << MPU_CTRL_HFNMIENA_Pos)                 /*!< MPU CTRL: HFNMIENA Mask */

#define MPU_CTRL_ENABLE_Pos                 0                                             /*!< MPU CTRL: ENABLE Position */
#define MPU_CTRL_ENABLE_Msk                (1ul << MPU_CTRL_ENABLE_Pos)                   /*!< MPU CTRL: ENABLE Mask */

#define MPU_REGION_32B     (0b00100 << 1)
#define MPU_REGION_64B     (0b00101 << 1)
#define MPU_REGION_128B    (0b00110 << 1)
#define MPU_REGION_256B    (0b00111 << 1)
#define MPU_REGION_512B    (0b01000 << 1)
#define MPU_REGION_1KB     (0b01001 << 1)
#define MPU_REGION_2KB     (0b01010 << 1)
#define MPU_REGION_4KB     (0b01011 << 1)
#define MPU_REGION_8KB     (0b01100 << 1)
#define MPU_REGION_16KB    (0b01101 << 1)
#define MPU_REGION_32KB    (0b01110 << 1)
#define MPU_REGION_64KB    (0b01111 << 1)
#define MPU_REGION_128KB   (0b10000 << 1)
#define MPU_REGION_256KB   (0b10001 << 1)
#define MPU_REGION_512KB   (0b10010 << 1)
#define MPU_REGION_1MB     (0b10011 << 1)
#define MPU_REGION_2MB     (0b10100 << 1)
#define MPU_REGION_4MB     (0b10101 << 1)
#define MPU_REGION_8MB     (0b10110 << 1)
#define MPU_REGION_16MB    (0b10111 << 1)
#define MPU_REGION_32MB    (0b11000 << 1)
#define MPU_REGION_64MB    (0b11001 << 1)
#define MPU_REGION_128MB   (0b11010 << 1)
#define MPU_REGION_256MB   (0b11011 << 1)
#define MPU_REGION_512MB   (0b11100 << 1)
#define MPU_REGION_1GB     (0b11101 << 1)
#define MPU_REGION_2GB     (0b11110 << 1)
#define MPU_REGION_4GB     (0b11111 << 1)

#define MPU_REGION_Valid   (1U << 4)

#define MPU_REGION_Enabled 1U
#define MPU_NO_EXEC (1U << 28)

#define MPU_No_access (0U << 24)
#define MPU_RW_No_access (1U << 24)
#define MPU_RW_RO (2U << 24)
#define MPU_RW (3U << 24)
#define MPU_RO_No_access (5U << 24)
#define MPU_RO (6U << 24)
/*
    http://www.st.com/web/en/resource/technical/document/programming_manual/CD00228163.pdf
    http://infocenter.arm.com/help/topic/com.arm.doc.faqs/ka16220.html
    http://infocenter.arm.com/help/topic/com.arm.doc.dai0179b/CHDFDFIG.html
    http://infocenter.arm.com/help/topic/com.arm.doc.ddi0363g/Bgbcdeca.html
    https://blog.feabhas.com/2013/02/setting-up-the-cortex-m34-armv7-m-memory-protection-unit-mpu/
    https://libopencm3.github.io/docs/latest/cm3/html/mpu_8h.html
*/
/*   Address range          Name       Type and Attributes   */
/*   0x00000000-0x1FFFFFFF  Code       Normal Cacheable, Write-Through, Allocate on read miss */
/*   0x20000000-0x3FFFFFFF  SRAM       Normal Cacheable, Write-Back, Allocate on read and write miss */
/*   0x40000000-0x5FFFFFFF  Peripheral Device, Non-shareable */
/*   0x60000000-0x7FFFFFFF  RAM        Normal Cacheable, Write-Back, Allocate on read and write miss */
/*   0x80000000-0x9FFFFFFF  RAM        Normal Cacheable, Write-Through, Allocate on read miss */
/*   0xA0000000-0xBFFFFFFF  Device     Device, Shareable */
/*   0xC0000000-0xDFFFFFFF  Device     Device, Non-shareable */
/*   0xE0000000-0xFFFFFFFF  (System space, of which: */

/*     0xE0000000-0xE000FFFF   Private Peripheral Bus   Strongly Ordered */
/*     0xE0010000-0xFFFFFFFF   Vendor System            Device (Non-shareable)   */

#define MPU_FLASH_RAM    (0b000100U << 16)
#define MPU_INTERNAL_RAM (0b000110U << 16)
#define MPU_EXTERNAL_RAM (0b000111U << 16)
#define MPU_PERIPHERIALS (0b000101U << 16)

static const uint32_t table[] = {
(0x08000000 | MPU_REGION_Valid | 0), (MPU_REGION_Enabled | MPU_FLASH_RAM | MPU_REGION_1MB | MPU_RO),
(0x20000000 | MPU_REGION_Valid | 1), (MPU_REGION_Enabled | MPU_INTERNAL_RAM | MPU_REGION_128KB | MPU_RW),
(0x40000000 | MPU_REGION_Valid | 2), (MPU_REGION_Enabled | MPU_NO_EXEC | MPU_PERIPHERIALS | MPU_REGION_512MB | MPU_RW),
(0x60000000 | MPU_REGION_Valid | 3), (MPU_REGION_Enabled | MPU_NO_EXEC | MPU_EXTERNAL_RAM | MPU_REGION_512MB | MPU_No_access),
(0x80000000 | MPU_REGION_Valid | 4), (MPU_REGION_Enabled | MPU_NO_EXEC | MPU_EXTERNAL_RAM | MPU_REGION_512MB | MPU_No_access),
(0xA0000000 | MPU_REGION_Valid | 5), (MPU_REGION_Enabled | MPU_NO_EXEC | MPU_PERIPHERIALS | MPU_REGION_512MB | MPU_No_access),
(0xE0000000 | MPU_REGION_Valid | 6), (MPU_REGION_Enabled | MPU_NO_EXEC | MPU_PERIPHERIALS | MPU_REGION_512MB | MPU_RW_No_access),
};

void mpu_init(void) {
  int i;
  /* Disable MPU */
  MPU->CTRL = 0;
  for(i=0;i<(sizeof(table)/sizeof(int));i+=2) {
    MPU->RBAR = table[i];
    MPU->RASR = table[i+1];
  }
  /* Enable MPU */
  SCB->SHCSR |= SCB_SHCSR_MEMFAULTENA_Msk;
  MPU->CTRL = MPU_CTRL_HFNMIENA_Msk | MPU_CTRL_PRIVDEFENA_Msk | MPU_CTRL_ENABLE_Msk;
  __ISB();
  __DSB();
}

void test(void) {
  mpu_init();
  volatile int test = *((int*)0x40020418);
  (void) test;
  while(1);
}
