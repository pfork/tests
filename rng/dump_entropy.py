#!/usr/bin/env python

# #!#!#!#!#!#!#!#!#!#!#!#!#!#!#
# need to call init()
# before using
# #!#!#!#!#!#!#!#!#!#!#!#!#!#!#

import sys
import usb.core
import usb.util
from binascii import hexlify
from struct import unpack

idVendor=0x0483
idProduct=0x5740
DEBUG = False

if DEBUG:
    import os
    os.environ['PYUSB_DEBUG_LEVEL'] = 'warning' # 'debug'
    os.environ['PYUSB_DEBUG'] = 'warning' # 'debug'

USB_CRYPTO_EP_CTRL_IN = 0x01
USB_CRYPTO_EP_DATA_IN = 0x02
USB_CRYPTO_EP_CTRL_OUT = 0x81
USB_CRYPTO_EP_DATA_OUT = 0x82

# USB endpoint cache
eps={}

def split_by_n( seq, n ):
    """A generator to divide a sequence into chunks of n units.
       src: http://stackoverflow.com/questions/9475241/split-python-string-every-nth-character"""
    while seq:
        yield seq[:n]
        seq = seq[n:]

def entropy():
    #reset()
    res = ''
    while(1):
        #res=eps[USB_CRYPTO_EP_DATA_OUT].read(65536)
        res=eps[USB_CRYPTO_EP_DATA_OUT].read(64)
        print ''.join([chr(x) for x in res])
        #for x in split_by_n(res,4):
        #    x = unpack("I", x)[0]
        #    while(1):
        #        try:
        #            print x
        #            break
        #        except:
        #            sys.stderr.write('.')
    #reset()

#####  support ops  #####
def init():
    dev = usb.core.find(idVendor=idVendor, idProduct=idProduct)
    cfg = dev.get_active_configuration()
    interface_number = cfg[(0,0)].bInterfaceNumber
    intf = usb.util.find_descriptor(cfg, bInterfaceNumber = interface_number)
    for ep in intf:
        eps[ep.bEndpointAddress]=ep

def reset():
    flush(USB_CRYPTO_EP_DATA_OUT)
    flush(USB_CRYPTO_EP_CTRL_OUT)

def flush(ep, quiet=True):
    while(True):
        try:
            tmp = eps[ep].read(64, timeout=10)
        except usb.core.USBError:
            break
        if not quiet: print '>', len(tmp), repr(''.join([chr(x) for x in tmp]))

init()

if __name__ == '__main__':
    #print flush(USB_CRYPTO_EP_CTRL_IN, False)
    #print flush(USB_CRYPTO_EP_CTRL_OUT, False)
    #print flush(USB_CRYPTO_EP_DAT_IN, False)
    #print flush(USB_CRYPTO_EP_DATA_OUT, False)
    #print repr(read_ctrl())
    entropy()
