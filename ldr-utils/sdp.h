/*
 * File: sdp.h
 *
 * Copyright 2006-2011 Analog Devices Inc.
 * Licensed under the GPL-2, see the file COPYING in this dir
 *
 * Description:
 * View LDR contents; based on the "Visual DSP++ 4.0 Loader Manual"
 * and misc Blackfin HRMs
 */

#ifndef __SDP_H__
#define __SDP_H__

#define ADI_SDP_USB_VID                0x0456
#define ADI_SDP_USB_PID                0xb630

#define ADI_SDP_WRITE_ENDPOINT         0x06
#define ADI_SDP_READ_ENDPOINT          0x05

#define ADI_SDP_CMD_GROUP_BASE         0xCA000000
#define ADI_SDP_CMD_FLASH_LED          (ADI_SDP_CMD_GROUP_BASE | 0x01)
#define ADI_SDP_CMD_GET_FW_VERSION     (ADI_SDP_CMD_GROUP_BASE | 0x02)
#define ADI_SDP_CMD_SDRAM_PROGRAM_BOOT (ADI_SDP_CMD_GROUP_BASE | 0x03)
#define ADI_SDP_CMD_READ_ID_EEPROMS    (ADI_SDP_CMD_GROUP_BASE | 0x04)
#define ADI_SDP_CMD_RESET_BOARD        (ADI_SDP_CMD_GROUP_BASE | 0x05)
#define ADI_SDP_CMD_READ_MAC_ADDRESS   (ADI_SDP_CMD_GROUP_BASE | 0x06)
#define ADI_SDP_CMD_STOP_STREAM        (ADI_SDP_CMD_GROUP_BASE | 0x07)

#define ADI_SDP_CMD_GROUP_USER         0xF8000000
#define ADI_SDP_CMD_USER_GET_GUID      (ADI_SDP_CMD_GROUP_USER | 0x01)
#define ADI_SDP_CMD_USER_MAX           (ADI_SDP_CMD_GROUP_USER | 0xFF)

#define ADI_SDP_SDRAM_PKT_MIN_LEN      512
#define ADI_SDP_SDRAM_PKT_MAX_CNT      0x400
#define ADI_SDP_SDRAM_PKT_MAX_LEN      0x10000

struct sdp_header {
	u32 cmd;         /* ADI_SDP_CMD_XXX */
	u32 out_len;     /* number bytes we transmit to sdp */
	u32 in_len;      /* number bytes sdp transmits to us */
	u32 num_params;  /* num params we're passing */
	u32 params[124]; /* params array */
};

struct sdp_version {
	struct rev {
		u16 major;
		u16 minor;
		u16 host;
		u16 bfin;
	} rev;
	u8 datestamp[12];
	u8 timestamp[8];
	u16 bmode;
	u16 flags;
};

#endif
