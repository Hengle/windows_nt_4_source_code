// returns pointers pOutStr
TCHAR *DecodeIRDA(int  *pFrameType,// return frame type (-1 = bad frame)
				BYTE *pFrameBuf, // pointer to buffer containing IRLAP frame
				UINT FrameLen, 	 // length of buffer 
				TCHAR *pOutStr,	 // string where decode packet is placed	
                UINT DecodeLayer,// 2-LAP only, 3-LAP/LMP, 4-LAP/LMP/TTP
				BOOL fNoConnAddr,// TRUE->Don't show connection address in str
                int DispMode    // DISP_ASCII/HEX/BOTH
);

#define DISP_ASCII  1
#define DISP_HEX    2
#define DISP_BOTH   3

extern int BaudBitMask;

#define IRLAP_BOF 0xC0
#define IRLAP_EOF 0xC1
#define IRLAP_ESC 0x7D
#define IRLAP_COMP_BIT 0x20 

#define IRLAP_BROADCAST      0xfe
#define _IRLAP_CMD           0x01
#define _IRLAP_RSP           0x00

#define IRLAP_I_FRM          0x00
#define IRLAP_S_FRM          0x01
#define IRLAP_U_FRM          0x03

/*
** Unnumbered Frame types with P/F bit set to 0
*/
#define IRLAP_UI             0x03
#define IRLAP_XID_CMD        0x2f
#define IRLAP_TEST           0xe3
#define IRLAP_SNRM           0x83
#define IRLAP_DISC           0x43
#define IRLAP_UA             0x63
#define IRLAP_FRMR           0x87
#define IRLAP_DM             0x0f
#define IRLAP_XID_RSP        0xaf

/*
** Supervisory Frames
*/
#define IRLAP_RR             0x01
#define IRLAP_RNR            0x05
#define IRLAP_REJ            0x09
#define IRLAP_SREJ           0x0d


#define IRLAP_GET_ADDR(addr)       (addr >> 1)
#define IRLAP_GET_CRBIT(addr)      (addr & 1) 
#define IRLAP_GET_PFBIT(cntl)      ((cntl >>4) & 1)
#define IRLAP_GET_UCNTL(cntl)      (cntl & 0xEF)
#define IRLAP_GET_SCNTL(cntl)      (cntl & 0x0F)
#define IRLAP_FRAME_TYPE(cntl)     (cntl & 0x01 ? (cntl & 3) : 0)
#define IRLAP_GET_NR(cntl)         ((cntl & 0xE0) >> 5)
#define IRLAP_GET_NS(cntl)         ((cntl & 0xE) >> 1)     

/*
** XID stuff
*/
#define XID_DISCV_FORMAT_ID     0x01
#define XID_NEGPARMS_FORMAT_ID 0x02

typedef struct 
{
    BYTE    SrcAddr[4];
    BYTE    DestAddr[4];
    BYTE    NoOfSlots:2;
    BYTE    GenNewAddr:1;
    BYTE    Reserved:5;
    BYTE    SlotNo;
    BYTE    Version;
} XID_DISCV_FORMAT;

/*
** SNRM
*/
typedef struct
{
    BYTE     SrcAddr[4];
    BYTE     DestAddr[4];
    BYTE     ConnAddr;
    BYTE     FirstPI;
} SNRM_FORMAT;

/*
** UA
*/
typedef struct
{
    BYTE     SrcAddr[4];
    BYTE     DestAddr[4];
    BYTE     FirstPI;
} UA_FORMAT;

/*
** LM-PDU stuff
*/
typedef struct
{
    BYTE    DLSAP_SEL:7;    
    BYTE    CntlBit:1;
    BYTE    SLSAP_SEL:7;
    BYTE    RsvrdBi1:1;    
} LM_HEADER;

/* LM-PDU frame types */
#define LM_PDU_CNTL_FRAME 1
#define LM_PDU_DATA_FRAME 0

typedef struct
{
    BYTE    OpCode:7;
    BYTE    ABit:1;
} LM_CNTL_FORMAT;

/* Opcodes */
#define LM_PDU_CONNECT    1
#define LM_PDU_DISCONNECT 2
#define LM_PDU_ACCESSMODE 3

#define LM_PDU_REQUEST 0
#define LM_PDU_CONFIRM 1

#define LM_PDU_SUCCESS        0
#define LM_PDU_FAILURE        1
#define LM_PDU_UNSUPPORTED 0xFF

#define LM_PDU_MULTIPLEXED 0
#define LM_PDU_EXCLUSIVE   1

/* Max disconnect reason code, see _LM_PDU_DscReason[] in decdirda.c */
#define LM_PDU_MAX_DSC_REASON  0x8

/*
** Negotiation Parameter Identifiers
*/
#define NEG_PI_BAUD        0x01
#define NEG_PI_MAX_TAT     0x82
#define NEG_PI_DATA_SZ     0x83
#define NEG_PI_WIN_SZ      0x84
#define NEG_PI_BOFS        0x85
#define NEG_PI_MIN_TAT     0x86
#define NEG_PI_DISC_THRESH 0x08

// Tiny TP!

#define TTP_PFLAG_NO_PARMS  0
#define TTP_PFLAG_PARMS     1

#define TTP_MBIT_NOT_FINAL  1
#define TTP_MBIT_FINAL      0

typedef struct
{
    BYTE    InitialCredit : 7;
    BYTE    ParmFlag      : 1;
} TTP_CONN_HEADER;

typedef struct
{
    BYTE    AdditionalCredit : 7;
    BYTE    MoreBit          : 1;
} TTP_DATA_HEADER;

#define net_short(x) 		((((x)&0xff) << 8) | (((x)&0xff00) >> 8))

#define net_long(x) 		(((((DWORD UNALIGNED)(x))&0xffL)<<24) | \
							((((DWORD UNALIGNED)(x))&0xff00L)<<8) | \
							((((DWORD UNALIGNED)(x))&0xff0000L)>>8) | \
							((((DWORD UNALIGNED)(x))&0xff000000L)>>24))
