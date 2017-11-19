# Common routines used by ADI ADSP-SC58x and ADSP-SC57x boards

proc smpu_config { smpu } {
   # Use SMPU instances to disable accesses to system memory that may not be
   # populated or needs to be initialized before being accessed. This will
   # avoid the possibility of an infinite stall in the system due to a
   # speculative access to a disabled or uninitialized memory. This is also
   # part of the workaround for silicon anomaly 20000018.

   if { $smpu == 0 } {
      set smpu_baseaddr 0x31007000
   } elseif { $smpu == 8 } {
      set smpu_baseaddr 0x31099000
   } elseif { $smpu == 9 } {
      set smpu_baseaddr 0x310a0000
   } elseif { $smpu == 10 } {
      set smpu_baseaddr 0x310a1000
   } else {
      puts stderr "Error: unknown SMPU number"
      shutdown error
   }

   set smpu_ctl			[expr {$smpu_baseaddr + 0x0}]
   set smpu_securectl	[expr {$smpu_baseaddr + 0x800}]

   # SMC - SMPU instance 0
   # *pREG_SMPU0_CTL |= ENUM_SMPU_CTL_RSDIS;
   pmmw $smpu_ctl 0x1 0

   # *pREG_SMPU0_SECURECTL = 0xf01;
   mww phys $smpu_securectl 0xf01
}

proc adspsc5xx_init_ddr3 { dmc } {
   global CHIPNAME

   if { $dmc == 0 } {
      set dmc_baseaddr 0x31070000
      set dummy_addr 0x80000000
   } else {
      set dmc_baseaddr 0x31073000
      set dummy_addr 0xc0000000
   }

   set dmc_ctl			[expr {$dmc_baseaddr + 0x4}]
   set dmc_stat			[expr {$dmc_baseaddr + 0x8}]
   set dmc_cfg			[expr {$dmc_baseaddr + 0x40}]
   set dmc_tr0			[expr {$dmc_baseaddr + 0x44}]
   set dmc_tr1			[expr {$dmc_baseaddr + 0x48}]
   set dmc_tr2			[expr {$dmc_baseaddr + 0x4c}]
   set dmc_mr			[expr {$dmc_baseaddr + 0x60}]
   set dmc_emr1			[expr {$dmc_baseaddr + 0x64}]
   set dmc_emr2			[expr {$dmc_baseaddr + 0x68}]
   set dmc_dllctl		[expr {$dmc_baseaddr + 0x80}]
   set dmc_phy_ctl0		[expr {$dmc_baseaddr + 0x1000}]
   set dmc_phy_ctl1		[expr {$dmc_baseaddr + 0x1004}]
   set dmc_phy_ctl2		[expr {$dmc_baseaddr + 0x1008}]
   set dmc_phy_ctl3		[expr {$dmc_baseaddr + 0x100c}]
   set dmc_phy_ctl4		[expr {$dmc_baseaddr + 0x1010}]
   set dmc_cal_padctl0	[expr {$dmc_baseaddr + 0x1034}]
   set dmc_cal_padctl2	[expr {$dmc_baseaddr + 0x103c}]

   # Configure SMPU (silicon anomaly 20000018)
   if { $CHIPNAME == "adspsc589" } {
      smpu_config 0
      smpu_config 8
   } elseif { $CHIPNAME == "adspsc573" } {
      smpu_config 0
   }

   # Set the RESETDLL bit (bit 11 of the DMC_PHY_CTL0 register) before CGU Initialization.
   # *pREG_DMC0_PHY_CTL0 |= BITM_DMC_PHY_CTL0_RESETDLL;
   pmmw $dmc_phy_ctl0 0x800 0

   # Set CGU clock select register to CLKO2/4 (ARM core)
   mww phys 0x3108d010 4

   # Reset processor to default power settings
   # Clear DISABLE and set EXIT_ACTIVE in CGU0_PLLCTL
   mww phys 0x3108d004 0x2
   # Set DF = 0 MSEL = 0x12 in CGU0_CTL
   mww phys 0x3108d000 0x1200
   # Set SYSSEL = 2 S0SEL = 2 S1SEL = 2 CSEL = 1 DSEL = 1 in CGU0_DIV
   mww phys 0x3108d00c 0x44014241

   # Read CGU0_STAT to make sure it's in FULL ON mode
   #mdw phys 0x3108d008

   # Clear the PHY_CTL0 after CGU Initialization
   mww phys $dmc_phy_ctl0 0

   # Wait for DLL lock - 9000 DCLK cycles
   # 1ms should be enough
   after 1

   # *pREG_DMC0_PHY_CTL0 |= 0x0000000F;
   pmmw $dmc_phy_ctl0 0xf 0
   # *pREG_DMC0_PHY_CTL2 |= 0xFC000000;
   pmmw $dmc_phy_ctl2 0xfc000000 0
   # *pREG_DMC0_PHY_CTL3 |= 0x0A0000C0;
   pmmw $dmc_phy_ctl3 0xa0000c0 0

   # *pREG_DMC0_PHY_CTL1 = 0x00000000;
   mww phys $dmc_phy_ctl1 0

   # *pREG_DMC0_PHY_CTL4 = 0x00000000;
   mww phys $dmc_phy_ctl4 0

   # Program the PAD RTT and driver impedance values required here
   # *pREG_DMC0_CAL_PADCTL0 = 0xE0000000;
   mww phys $dmc_cal_padctl0 0xe0000000
   # *pREG_DMC0_CAL_PADCTL2 = 0x0078283C;
   mww phys $dmc_cal_padctl2 0x0078283c

   # Start calibration
   # *pREG_DMC0_CAL_PADCTL0 |= 0x10000000;
   pmmw $dmc_cal_padctl0 0x10000000 0

   # Wait for PAD calibration to complete - 300 DCLK cycle.
   # 1ms should be enough
   after 1

   # *pREG_DMC0_CFG = 0x00000522;
   mww phys $dmc_cfg 0x00000522
   # *pREG_DMC0_TR0 = 0x41711646;
   mww phys $dmc_tr0 0x41711646
   # *pREG_DMC0_TR1 = 0x40480db6;
   mww phys $dmc_tr1 0x40480db6
   # *pREG_DMC0_TR2 = 0x00347417;
   mww phys $dmc_tr2 0x00347417
   # *pREG_DMC0_MR = 0x00000D30;
   mww phys $dmc_mr 0x00000D30
   # *pREG_DMC0_EMR1 = 0x00000006;
   mww phys $dmc_emr1 0x00000006
   # *pREG_DMC0_EMR2 = 0x00000008;
   mww phys $dmc_emr2 0x00000008
   # *pREG_DMC0_CTL = 0x00000405;
   mww phys $dmc_ctl 0x00000405

   # Wait till INIDONE is set
   # while((*pREG_DMC0_STAT&BITM_DMC_STAT_INITDONE)==0);
   set data 0
   while { [expr {$data & 4}] == 0 } {
      set data [pmemread32 $dmc_stat]
   }

   # *pREG_DMC0_DLLCTL = 0x00000948;
   mww phys $dmc_dllctl 0x00000948

   # Workaround for silicon anomaly 20000037
   # Dummy read
   set data [memread32 $dummy_addr]
   # *pREG_DMC0_PHY_CTL0|=0x1000;
   # *pREG_DMC0_PHY_CTL0&=~0x1000;
   set data [pmemread32 $dmc_phy_ctl0]
   mww phys $dmc_phy_ctl0 [expr {$data | 0x1000}]
   mww phys $dmc_phy_ctl0 [expr {$data & ~0x1000}]
}

proc adspsc5xx_init_ddr2 { } {
   set dmc_baseaddr 0x31070000
   set dummy_addr 0x80000000

   set dmc_ctl			[expr {$dmc_baseaddr + 0x4}]
   set dmc_stat			[expr {$dmc_baseaddr + 0x8}]
   set dmc_cfg			[expr {$dmc_baseaddr + 0x40}]
   set dmc_tr0			[expr {$dmc_baseaddr + 0x44}]
   set dmc_tr1			[expr {$dmc_baseaddr + 0x48}]
   set dmc_tr2			[expr {$dmc_baseaddr + 0x4c}]
   set dmc_mr			[expr {$dmc_baseaddr + 0x60}]
   set dmc_emr1			[expr {$dmc_baseaddr + 0x64}]
   set dmc_emr2			[expr {$dmc_baseaddr + 0x68}]
   set dmc_dllctl		[expr {$dmc_baseaddr + 0x80}]
   set dmc_phy_ctl0		[expr {$dmc_baseaddr + 0x1000}]
   set dmc_phy_ctl1		[expr {$dmc_baseaddr + 0x1004}]
   set dmc_phy_ctl2		[expr {$dmc_baseaddr + 0x1008}]
   set dmc_phy_ctl3		[expr {$dmc_baseaddr + 0x100c}]
   set dmc_phy_ctl4		[expr {$dmc_baseaddr + 0x1010}]
   set dmc_cal_padctl0	[expr {$dmc_baseaddr + 0x1034}]
   set dmc_cal_padctl2	[expr {$dmc_baseaddr + 0x103c}]

   # Configure SMPU (silicon anomaly 20000018)
   smpu_config 0
   smpu_config 8
   smpu_config 10

   # Set the RESETDLL bit (bit 11 of the DMC_PHY_CTL0 register) before CGU Initialization.
   # *pREG_DMC0_PHY_CTL0 |= BITM_DMC_PHY_CTL0_RESETDLL;
   pmmw $dmc_phy_ctl0 0x800 0

   # Set CGU clock select register to CLKO2/4 (ARM core)
   mww phys 0x3108d010 4

   # Reset processor to default power settings
   # Clear DISABLE and set EXIT_ACTIVE in CGU0_PLLCTL
   mww phys 0x3108d004 0x2
   # Set DF = 0 MSEL = 0x10 in CGU0_CTL
   mww phys 0x3108d000 0x1000
   # Set SYSSEL = 2 S0SEL = 2 S1SEL = 2 CSEL = 1 DSEL = 1 in CGU0_DIV
   mww phys 0x3108d00c 0x44014241

   # Read CGU0_STAT to make sure it's in FULL ON mode
   #mdw phys 0x3108d008

   # Clear the PHY_CTL0 after CGU Initialization
   mww phys $dmc_phy_ctl0 0

   # Wait for DLL lock - 9000 DCLK cycles
   # 1ms should be enough
   after 1

   # *pREG_DMC0_PHY_CTL0 |= 0x0000000F;
   pmmw $dmc_phy_ctl0 0xf 0
   # *pREG_DMC0_PHY_CTL2 |= 0xFC000000;
   pmmw $dmc_phy_ctl2 0xfc000000 0
   # *pREG_DMC0_PHY_CTL3 |= 0x0A0000C0;
   pmmw $dmc_phy_ctl3 0xa0000c0 0

   # *pREG_DMC0_PHY_CTL1 = 0x00000000;
   mww phys $dmc_phy_ctl1 0

   # *pREG_DMC0_PHY_CTL4 = 0x00000001;
   mww phys $dmc_phy_ctl4 1

   # Program the PAD RTT and driver impedance values required here
   # *pREG_DMC0_CAL_PADCTL0 = 0xE0000000;
   mww phys $dmc_cal_padctl0 0xe0000000
   # *pREG_DMC0_CAL_PADCTL2 = 0x0078283C;
   mww phys $dmc_cal_padctl2 0x0078283c

   # Start calibration
   # *pREG_DMC0_CAL_PADCTL0 |= 0x10000000;
   pmmw $dmc_cal_padctl0 0x10000000 0

   # Wait for PAD calibration to complete - 300 DCLK cycle.
   # 1ms should be enough
   after 1

   # *pREG_DMC0_CFG = 0x00000522;
   mww phys $dmc_cfg 0x00000522
   # *pREG_DMC0_TR0 = 0x21610535;
   mww phys $dmc_tr0 0x21610535
   # *pREG_DMC0_TR1 = 0x404e0c30;
   mww phys $dmc_tr1 0x404e0c30
   # *pREG_DMC0_TR2 = 0x00326312;
   mww phys $dmc_tr2 0x00326312
   # *pREG_DMC0_MR = 0x00000a52;
   mww phys $dmc_mr 0x00000a52
   # *pREG_DMC0_EMR1 = 0x00000004;
   mww phys $dmc_emr1 0x00000004
   # *pREG_DMC0_EMR2 = 0x00000000;
   mww phys $dmc_emr2 0x00000000
   # *pREG_DMC0_CTL = 0x00000404;
   mww phys $dmc_ctl 0x00000404

   # Wait till INIDONE is set
   # while((*pREG_DMC0_STAT&BITM_DMC_STAT_INITDONE)==0);
   set data 0
   while { [expr {$data & 4}] == 0 } {
      set data [pmemread32 $dmc_stat]
   }

   # *pREG_DMC0_DLLCTL = 0x00000948;
   mww phys $dmc_dllctl 0x00000948

   # Workaround for silicon anomaly 20000037
   # Dummy read
   set data [memread32 $dummy_addr]
   # *pREG_DMC0_PHY_CTL0|=0x1000;
   # *pREG_DMC0_PHY_CTL0&=~0x1000;
   set data [pmemread32 $dmc_phy_ctl0]
   mww phys $dmc_phy_ctl0 [expr {$data | 0x1000}]
   mww phys $dmc_phy_ctl0 [expr {$data & ~0x1000}]
}
