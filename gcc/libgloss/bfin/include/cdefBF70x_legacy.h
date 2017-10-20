/************************************************************************
 *
 * cdefBF70x_legacy.h
 *
 * (c) Copyright 2014 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

#ifndef _WRAP_CDEF_BF70X_LEGACY_H
#define _WRAP_CDEF_BF70X_LEGACY_H

#include <sys/defBF70x_legacy.h>

/* L1DM Legacy Registers */

#define pSRAM_BASE_ADDRESS               ((void * volatile *)SRAM_BASE_ADDRESS)                  /* SRAM Base Address */
#define pDMEM_CONTROL                    ((volatile uint32_t *)DMEM_CONTROL)                     /* Data memory control */
#define pDCPLB_STATUS                    ((volatile uint32_t *)DCPLB_STATUS)                     /* Data Cacheability Protection Lookaside Buffer Status */
#define pDCPLB_FAULT_ADDR                ((void * volatile *)DCPLB_FAULT_ADDR)                   /* Data Cacheability Protection Lookaside Buffer Fault Address */
#define pDCPLB_ADDR0                     ((void * volatile *)DCPLB_ADDR0)                        /* Cacheability Protection Lookaside Buffer Descriptor Address */
#define pDCPLB_ADDR1                     ((void * volatile *)DCPLB_ADDR1)                        /* Cacheability Protection Lookaside Buffer Descriptor Address */
#define pDCPLB_ADDR2                     ((void * volatile *)DCPLB_ADDR2)                        /* Cacheability Protection Lookaside Buffer Descriptor Address */
#define pDCPLB_ADDR3                     ((void * volatile *)DCPLB_ADDR3)                        /* Cacheability Protection Lookaside Buffer Descriptor Address */
#define pDCPLB_ADDR4                     ((void * volatile *)DCPLB_ADDR4)                        /* Cacheability Protection Lookaside Buffer Descriptor Address */
#define pDCPLB_ADDR5                     ((void * volatile *)DCPLB_ADDR5)                        /* Cacheability Protection Lookaside Buffer Descriptor Address */
#define pDCPLB_ADDR6                     ((void * volatile *)DCPLB_ADDR6)                        /* Cacheability Protection Lookaside Buffer Descriptor Address */
#define pDCPLB_ADDR7                     ((void * volatile *)DCPLB_ADDR7)                        /* Cacheability Protection Lookaside Buffer Descriptor Address */
#define pDCPLB_ADDR8                     ((void * volatile *)DCPLB_ADDR8)                        /* Cacheability Protection Lookaside Buffer Descriptor Address */
#define pDCPLB_ADDR9                     ((void * volatile *)DCPLB_ADDR9)                        /* Cacheability Protection Lookaside Buffer Descriptor Address */
#define pDCPLB_ADDR10                    ((void * volatile *)DCPLB_ADDR10)                       /* Cacheability Protection Lookaside Buffer Descriptor Address */
#define pDCPLB_ADDR11                    ((void * volatile *)DCPLB_ADDR11)                       /* Cacheability Protection Lookaside Buffer Descriptor Address */
#define pDCPLB_ADDR12                    ((void * volatile *)DCPLB_ADDR12)                       /* Cacheability Protection Lookaside Buffer Descriptor Address */
#define pDCPLB_ADDR13                    ((void * volatile *)DCPLB_ADDR13)                       /* Cacheability Protection Lookaside Buffer Descriptor Address */
#define pDCPLB_ADDR14                    ((void * volatile *)DCPLB_ADDR14)                       /* Cacheability Protection Lookaside Buffer Descriptor Address */
#define pDCPLB_ADDR15                    ((void * volatile *)DCPLB_ADDR15)                       /* Cacheability Protection Lookaside Buffer Descriptor Address */
#define pDCPLB_DATA0                     ((volatile uint32_t *)DCPLB_DATA0)                      /* Cacheability Protection Lookaside Buffer Descriptor Data */
#define pDCPLB_DATA1                     ((volatile uint32_t *)DCPLB_DATA1)                      /* Cacheability Protection Lookaside Buffer Descriptor Data */
#define pDCPLB_DATA2                     ((volatile uint32_t *)DCPLB_DATA2)                      /* Cacheability Protection Lookaside Buffer Descriptor Data */
#define pDCPLB_DATA3                     ((volatile uint32_t *)DCPLB_DATA3)                      /* Cacheability Protection Lookaside Buffer Descriptor Data */
#define pDCPLB_DATA4                     ((volatile uint32_t *)DCPLB_DATA4)                      /* Cacheability Protection Lookaside Buffer Descriptor Data */
#define pDCPLB_DATA5                     ((volatile uint32_t *)DCPLB_DATA5)                      /* Cacheability Protection Lookaside Buffer Descriptor Data */
#define pDCPLB_DATA6                     ((volatile uint32_t *)DCPLB_DATA6)                      /* Cacheability Protection Lookaside Buffer Descriptor Data */
#define pDCPLB_DATA7                     ((volatile uint32_t *)DCPLB_DATA7)                      /* Cacheability Protection Lookaside Buffer Descriptor Data */
#define pDCPLB_DATA8                     ((volatile uint32_t *)DCPLB_DATA8)                      /* Cacheability Protection Lookaside Buffer Descriptor Data */
#define pDCPLB_DATA9                     ((volatile uint32_t *)DCPLB_DATA9)                      /* Cacheability Protection Lookaside Buffer Descriptor Data */
#define pDCPLB_DATA10                    ((volatile uint32_t *)DCPLB_DATA10)                     /* Cacheability Protection Lookaside Buffer Descriptor Data */
#define pDCPLB_DATA11                    ((volatile uint32_t *)DCPLB_DATA11)                     /* Cacheability Protection Lookaside Buffer Descriptor Data */
#define pDCPLB_DATA12                    ((volatile uint32_t *)DCPLB_DATA12)                     /* Cacheability Protection Lookaside Buffer Descriptor Data */
#define pDCPLB_DATA13                    ((volatile uint32_t *)DCPLB_DATA13)                     /* Cacheability Protection Lookaside Buffer Descriptor Data */
#define pDCPLB_DATA14                    ((volatile uint32_t *)DCPLB_DATA14)                     /* Cacheability Protection Lookaside Buffer Descriptor Data */
#define pDCPLB_DATA15                    ((volatile uint32_t *)DCPLB_DATA15)                     /* Cacheability Protection Lookaside Buffer Descriptor Data */

/* end L1DM Legacy Register Macros */

/* L1IM Legacy Registers */

#define pIMEM_CONTROL                    ((volatile uint32_t *)IMEM_CONTROL)                     /* Instruction memory control */
#define pICPLB_STATUS                    ((volatile uint32_t *)ICPLB_STATUS)                     /* Cacheability Protection Lookaside Buffer Status */
#define pICPLB_FAULT_ADDR                ((void * volatile *)ICPLB_FAULT_ADDR)                   /* Cacheability Protection Lookaside Buffer Fault Address */
#define pICPLB_ADDR0                     ((void * volatile *)ICPLB_ADDR0)                        /* Cacheability Protection Lookaside Buffer Descriptor Address */
#define pICPLB_ADDR1                     ((void * volatile *)ICPLB_ADDR1)                        /* Cacheability Protection Lookaside Buffer Descriptor Address */
#define pICPLB_ADDR2                     ((void * volatile *)ICPLB_ADDR2)                        /* Cacheability Protection Lookaside Buffer Descriptor Address */
#define pICPLB_ADDR3                     ((void * volatile *)ICPLB_ADDR3)                        /* Cacheability Protection Lookaside Buffer Descriptor Address */
#define pICPLB_ADDR4                     ((void * volatile *)ICPLB_ADDR4)                        /* Cacheability Protection Lookaside Buffer Descriptor Address */
#define pICPLB_ADDR5                     ((void * volatile *)ICPLB_ADDR5)                        /* Cacheability Protection Lookaside Buffer Descriptor Address */
#define pICPLB_ADDR6                     ((void * volatile *)ICPLB_ADDR6)                        /* Cacheability Protection Lookaside Buffer Descriptor Address */
#define pICPLB_ADDR7                     ((void * volatile *)ICPLB_ADDR7)                        /* Cacheability Protection Lookaside Buffer Descriptor Address */
#define pICPLB_ADDR8                     ((void * volatile *)ICPLB_ADDR8)                        /* Cacheability Protection Lookaside Buffer Descriptor Address */
#define pICPLB_ADDR9                     ((void * volatile *)ICPLB_ADDR9)                        /* Cacheability Protection Lookaside Buffer Descriptor Address */
#define pICPLB_ADDR10                    ((void * volatile *)ICPLB_ADDR10)                       /* Cacheability Protection Lookaside Buffer Descriptor Address */
#define pICPLB_ADDR11                    ((void * volatile *)ICPLB_ADDR11)                       /* Cacheability Protection Lookaside Buffer Descriptor Address */
#define pICPLB_ADDR12                    ((void * volatile *)ICPLB_ADDR12)                       /* Cacheability Protection Lookaside Buffer Descriptor Address */
#define pICPLB_ADDR13                    ((void * volatile *)ICPLB_ADDR13)                       /* Cacheability Protection Lookaside Buffer Descriptor Address */
#define pICPLB_ADDR14                    ((void * volatile *)ICPLB_ADDR14)                       /* Cacheability Protection Lookaside Buffer Descriptor Address */
#define pICPLB_ADDR15                    ((void * volatile *)ICPLB_ADDR15)                       /* Cacheability Protection Lookaside Buffer Descriptor Address */
#define pICPLB_DATA0                     ((volatile uint32_t *)ICPLB_DATA0)                      /* Cacheability Protection Lookaside Buffer Descriptor Status */
#define pICPLB_DATA1                     ((volatile uint32_t *)ICPLB_DATA1)                      /* Cacheability Protection Lookaside Buffer Descriptor Status */
#define pICPLB_DATA2                     ((volatile uint32_t *)ICPLB_DATA2)                      /* Cacheability Protection Lookaside Buffer Descriptor Status */
#define pICPLB_DATA3                     ((volatile uint32_t *)ICPLB_DATA3)                      /* Cacheability Protection Lookaside Buffer Descriptor Status */
#define pICPLB_DATA4                     ((volatile uint32_t *)ICPLB_DATA4)                      /* Cacheability Protection Lookaside Buffer Descriptor Status */
#define pICPLB_DATA5                     ((volatile uint32_t *)ICPLB_DATA5)                      /* Cacheability Protection Lookaside Buffer Descriptor Status */
#define pICPLB_DATA6                     ((volatile uint32_t *)ICPLB_DATA6)                      /* Cacheability Protection Lookaside Buffer Descriptor Status */
#define pICPLB_DATA7                     ((volatile uint32_t *)ICPLB_DATA7)                      /* Cacheability Protection Lookaside Buffer Descriptor Status */
#define pICPLB_DATA8                     ((volatile uint32_t *)ICPLB_DATA8)                      /* Cacheability Protection Lookaside Buffer Descriptor Status */
#define pICPLB_DATA9                     ((volatile uint32_t *)ICPLB_DATA9)                      /* Cacheability Protection Lookaside Buffer Descriptor Status */
#define pICPLB_DATA10                    ((volatile uint32_t *)ICPLB_DATA10)                     /* Cacheability Protection Lookaside Buffer Descriptor Status */
#define pICPLB_DATA11                    ((volatile uint32_t *)ICPLB_DATA11)                     /* Cacheability Protection Lookaside Buffer Descriptor Status */
#define pICPLB_DATA12                    ((volatile uint32_t *)ICPLB_DATA12)                     /* Cacheability Protection Lookaside Buffer Descriptor Status */
#define pICPLB_DATA13                    ((volatile uint32_t *)ICPLB_DATA13)                     /* Cacheability Protection Lookaside Buffer Descriptor Status */
#define pICPLB_DATA14                    ((volatile uint32_t *)ICPLB_DATA14)                     /* Cacheability Protection Lookaside Buffer Descriptor Status */
#define pICPLB_DATA15                    ((volatile uint32_t *)ICPLB_DATA15)                     /* Cacheability Protection Lookaside Buffer Descriptor Status */

  /* end L1IM Legacy Register Macros */

#endif	/* end ifndef _WRAP_CDEF_BF70X_LEGACY_H */
