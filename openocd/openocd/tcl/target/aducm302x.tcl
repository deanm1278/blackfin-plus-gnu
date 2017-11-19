# Common file for Analog Devices ADuCM302x

# minimal dap memaccess values for adapter frequencies
#   1 MHz:  6
#   2 MHz:  8
#   5 MHz: 18
#   9 MHz: 27
#  15 MHz: 43
#  23 MHz: 74

# hardware has 2 breakpoints, 1 watchpoints

#
# ADuCM302x devices support only SWD transport.
#
transport select swd

source [find target/swj-dp.tcl]

set CPU_MAX_ADDRESS 0xFFFFFFFF
source [find bitsbytes.tcl]
source [find memory.tcl]
source [find mem_helper.tcl]

if { [info exists CHIPNAME] } {
   set _CHIPNAME $CHIPNAME
} else {
   set _CHIPNAME aducm302x
}

if { [info exists CHIPID] } {
   set _CHIPID $CHIPID
} else {
   puts stderr "Error: CHIPID is not defined"
   shutdown error
}

if { [info exists ENDIAN] } {
   set _ENDIAN $ENDIAN
} else {
   set _ENDIAN little
}

adapter_khz 1000

if { [info exists CPUTAPID] } {
   set _CPUTAPID $CPUTAPID
} else {
   set _CPUTAPID 0x2ba01477
}

swj_newdap $_CHIPNAME cpu -expected-id $_CPUTAPID

set _TARGETNAME $_CHIPNAME.cpu
target create $_TARGETNAME cortex_m -endian $_ENDIAN -chain-position $_TARGETNAME

if { [info exists WORKAREASIZE] } {
   set _WORKAREASIZE $WORKAREASIZE
} else {
   # default to 8K working area
   set _WORKAREASIZE 0x2000
}

$_TARGETNAME configure -work-area-phys 0x20000000 -work-area-size $_WORKAREASIZE

$_TARGETNAME configure -event reset-init {
   # disable watchdog, which will fire in about 32 second after reset.
   mwh 0x40002c08 0x0
   # After reset LR is 0xffffffff. There will be an error when GDB tries to
   # read from that address.
   reg lr 0
}

$_TARGETNAME configure -event examine-end {
   global _CHIPNAME
   global _CHIPID

   # read ADIID
   set sys_adiid 0x40002020
   set adiid [memread16 $sys_adiid]

   # read CHIPID
   set sys_chipid 0x40002024
   set chipid [memread16 $sys_chipid]

   if { $adiid != 0x4144 } {
      puts stderr "Error: not an Analog Devices Cortex-M based part"
      shutdown error
   }

   puts [format "Info : CHIPID 0x%04x" $chipid]

   if { [expr { $chipid & 0xfff0 } ] != $_CHIPID } {
      puts stderr "Error: not $_CHIPNAME part"
      shutdown error
   }
}
 
$_TARGETNAME configure -event gdb-attach {
   reset init 

   arm semihosting enable
}

$_TARGETNAME configure -event gdb-flash-erase-start {
   reset init
   mww 0x40018054 0x1
}

$_TARGETNAME configure -event gdb-flash-write-end {
   # get the reset handler address from application's vector table
   set reset_handler [memread32 4]

   reset init

   # run kernel and stop at the first instruction of application reset handler
   bp $reset_handler 2 hw
   resume
   wait_halt
   rbp $reset_handler
}

set _FLASHNAME $_CHIPNAME.flash
if { [info exists FLASHSIZE] } {
   set _FLASHSIZE $FLASHSIZE
} else {
   set _FLASHSIZE 0x40000
}
flash bank $_FLASHNAME aducm302x 0 $_FLASHSIZE 0 0 $_TARGETNAME

if {![using_hla]} {
   # if srst is not fitted use SYSRESETREQ to
   # perform a soft reset
   cortex_m reset_config sysresetreq
}
