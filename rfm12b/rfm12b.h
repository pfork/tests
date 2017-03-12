#ifndef _RFM12B_H
#define _RFM12B_H

unsigned char rfm_write_reg(unsigned char reg, unsigned char value);
unsigned char rfm_read_reg(unsigned char reg);
unsigned char rfm_read_buf(unsigned char reg, unsigned char *pBuf, unsigned char bytes);
unsigned char rfm_write_buf(unsigned char reg, unsigned char *pBuf, unsigned char bytes);
void nrf24_init(void);

#endif /*_RFM12B_H*/

