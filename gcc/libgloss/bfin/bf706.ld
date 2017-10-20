/*
 * The default linker script for standalone executables running on
 * ADSP-BF706 processor.
 *
 * Copyright (C) 2017 Dean Miller
 *
 * The authors hereby grant permission to use, copy, modify, distribute,
 * and license this software and its documentation for any purpose, provided
 * that existing copyright notices are retained in all copies and that this
 * notice is included verbatim in any distributions. No written agreement,
 * license, or royalty fee is required for any of the authorized uses.
 * Modifications to this software may be copyrighted by their authors
 * and need not follow the licensing terms described here, provided that
 * the new terms are clearly indicated on the first page of each file where
 * they apply.
 */

MEMORY
{
  MEM_L1_CODE : ORIGIN = 0x11A00000, LENGTH = 0xC000
  MEM_L1_CODE_CACHE : ORIGIN = 0x11A0C000, LENGTH = 0xC000
  MEM_L1_DATA_B : ORIGIN = 0x11900000, LENGTH = 0x8000
  MEM_L1_DATA_A : ORIGIN = 0x11800000, LENGTH = 0x8000

  MEM_L2_SCRATCH : ORIGIN = 0x08000000, LENGTH = 0x2000
  MEM_L2_SRAM : ORIGIN = 0x08002000, LENGTH = 0xEC000
  MEM_L2_SRAM_UNCACHED : ORIGIN = 0x080F0000, LENGTH = 0x10000
  MEM_L2_ROM : ORIGIN = 0x04010000, LENGTH = 0x70000
}
