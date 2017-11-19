dump_trace()
{
	local j n p skip func src line args a
	# ignore dump_trace, so start at 2
	skip=2

	log_echo
	log_echo "Backtrace:  (most recent call is first)"

	p=0
	for (( n = skip ; n < ${#FUNCNAME[@]} ; ++n )) ; do
		func=${FUNCNAME[${n} - 1]}
		src=${BASH_SOURCE[${n}]##*/}
		line=${BASH_LINENO[${n} - 1]}

		args=
		for (( j = 0 ; j < ${BASH_ARGC[${n} - 1]} ; ++j )); do
			a=${BASH_ARGV[$(( p + j ))]}
			args="'${a}' ${args}"
		done
		(( p += ${BASH_ARGC[${n} - 1]} ))

		log_printf '   #%i: File: %-20s   Line: %4i   Function: %s\n       Args: %s' \
			"$(( n - skip ))" "${src}" "${line}" "${func}" "${args}"
	done

	log_echo
}
