#include "debug.h"
#include "port.h"
#include "basic.h"

#define	DUARTxBase(x)		(0x21c0500 + 0x001000*(x-1))
#define DUARTx_URBRn(x,n)		(DUARTxBase(x) + 0x100*(n-1) + 0x00)
#define DUARTx_UTHRn(x,n)		(DUARTxBase(x) + 0x100*(n-1) + 0x00)
#define DUARTx_UDLBn(x,n)		(DUARTxBase(x) + 0x100*(n-1) + 0x00)
#define DUARTx_UDMBn(x,n)		(DUARTxBase(x) + 0x100*(n-1) + 0x01)
#define DUARTx_UIERn(x,n)		(DUARTxBase(x) + 0x100*(n-1) + 0x01)
#define DUARTx_UIIRn(x,n)		(DUARTxBase(x) + 0x100*(n-1) + 0x02)
#define DUARTx_UFCRn(x,n)		(DUARTxBase(x) + 0x100*(n-1) + 0x02)
#define DUARTx_UAFRn(x,n)		(DUARTxBase(x) + 0x100*(n-1) + 0x02)
#define DUARTx_ULCRn(x,n)		(DUARTxBase(x) + 0x100*(n-1) + 0x03)
#define DUARTx_UMCRn(x,n)		(DUARTxBase(x) + 0x100*(n-1) + 0x04)
#define DUARTx_ULSRn(x,n)		(DUARTxBase(x) + 0x100*(n-1) + 0x05)
#define DUARTx_UMSRn(x,n)		(DUARTxBase(x) + 0x100*(n-1) + 0x06)
#define DUARTx_USCRn(x,n)		(DUARTxBase(x) + 0x100*(n-1) + 0x07)
#define DUARTx_UDSRn(x,n)		(DUARTxBase(x) + 0x100*(n-1) + 0x10)

void serial_init(void){
    return;
}
B get_serial(){
    while(INB(DUARTx_UDSRn(1,1))&0x01){
        if((INB(DUARTx_UIIRn(1,1))&0xff)==0x6){
            INB(DUARTx_ULSRn(1,1));
        }
    }

    return INB(DUARTx_URBRn(1,1));
}

void put_serial(UB val){
    while((INB(DUARTx_UDSRn(1,1))&0x2)!=0);
    OUTB(DUARTx_UTHRn(1,1), val);
}
