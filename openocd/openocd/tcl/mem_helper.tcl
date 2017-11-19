# Helper for common memory read/modify/write procedures

# mrw: "memory read word", returns value of $reg
proc mrw {reg} {
	set value ""
	mem2array value 32 $reg 1
	return $value(0)
}

add_usage_text mrw "address"
add_help_text mrw "Returns value of word in memory."

# mmw: "memory modify word", updates value of $reg
#       $reg <== ((value & ~$clearbits) | $setbits)
proc mmw {reg setbits clearbits} {
	set old [mrw $reg]
	set new [expr ($old & ~$clearbits) | $setbits]
	mww $reg $new
}

add_usage_text mmw "address setbits clearbits"
add_help_text mmw "Modify word in memory. new_val = (old_val & ~clearbits) | setbits;"

# pmrw: "physical memory read word", returns value of $reg
proc pmrw {reg} {
	set value ""
	pmem2array value 32 $reg 1
	return $value(0)
}

add_usage_text pmrw "address"
add_help_text pmrw "Returns value of word in physical memory."

# pmmw: "physical memory modify word", updates value of $reg
#       $reg <== ((value & ~$clearbits) | $setbits)
proc pmmw {reg setbits clearbits} {
	set old [pmrw $reg]
	set new [expr ($old & ~$clearbits) | $setbits]
	mww phys $reg $new
}

add_usage_text pmmw "address setbits clearbits"
add_help_text pmmw "Modify word in physical memory. new_val = (old_val & ~clearbits) | setbits;"
