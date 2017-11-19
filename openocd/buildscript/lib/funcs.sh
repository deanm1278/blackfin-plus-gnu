#
# All random utility functions live here
#

# write the message to stderr and exit
error()
{
	printf "$*\n" 1>&2
	exit 1
}

# standard notice format used at start of script.  so this:
#	notice "This is a message" "result"
# will output something like:
#	This is a message:       result
# first argument is everything before the colon, while the rest
# goes after it in the result side
notice()
{
	local notice="$1"
	shift
	printf "%-28s%s\n" "$notice: " "$*"
}

# does $1 exist in the $2... list ?
has()
{
	local o="$1"; shift
	case " $* " in *" $o "*) return 0;; *) return 1;; esac
}

# make sure the specified directory is an absolute path
is_abs_file()
{
	[ -n "$1" ] && [ -f "/..$1" ]
}

# make sure the specified directory is an absolute path
is_abs_dir()
{
	[ -n "$1" ] && [ -d "/..$1" ]
}

# check a bunch of directories and abort if any of them are not absolute.
# first argument is optional and will issue a `notice` if not empty.
check_abs_dir()
{
	local msg="$1"
	shift

	local d
	for d in "$@" ; do
		if ! is_abs_dir "$d" ; then
			error "Directory does not exist, or is not an absolute path:\n  -- ${d}"
		fi
	done

	[ -n "${msg}" ] && notice "${msg}" "$@"
	return 0
}

check_src_pkg() {
	local var="`echo DIR_${2:-$1}_SOURCE | tr '[:lower:]' '[:upper:]' | sed 's:[-]:_:g'`"
	local val="${DIR_SOURCE}/$1"
	eval ${var}=\"${val}\"
	local abs_path=`realpath "${val}"`
	check_abs_dir "Path to $1" "${abs_path}"
}

# create the specified output dir and clean it if need be
mk_output_dir()
{
	notice "Path to $1 output dir" "$2"

	if [ ! -d "$2" ] ; then
		mkdir -p "$2"
		check_abs_dir "" "$2"
	else
		check_abs_dir "" "$2"

		if [ "x$3" = "x" ] && ! ${RESUME_BUILD} ; then
			echo "  directory already exists - cleaning"
			rm -rf $2/*
		fi
	fi
}

requote() { printf "'%s' " "$@"; }

print_stop_time()
{
	local END=$(date +%s)
	local DIFF=$((END - START))
	local h=$((DIFF / 3600))
	local m=$(((DIFF % 3600) / 60))
	local s=$((DIFF % 60))
	[ ${h} -ne 0 ] && printf "${h}h"
	[ ${m} -ne 0 ] && printf "${m}m"
	[ ${s} -ne 0 ] && printf "${s}s"
	[ ${h}${m}${s} -eq 0 ] && printf "0s"
}

die_with_log()
{
	(
	# save the tail before logging the backtrace
	out=$(tail "$ACTUAL_LOGFILE")
	dump_trace

	[ -n "$*" ] && printf '%b\n\n' "$*"
	echo "Please report an error to http://blackfin.uclinux.org/gf/project/toolchain"
	echo " Build error at `date`"
	echo "  occurred $(print_stop_time) into script"

	# Create a single file so users can email it to us,
	# we sleep, so things can finish writing to the log file
	sleep 1
	tout=$(tar -jhcf "$DIR_LOG"/config.logs.tbz2 $(find "$DIR_BUILD" -name "config.log") "$ACTUAL_LOGFILE" 2>&1)
	pid=$!

	echo "When reporting issue, we may ask for $DIR_LOG/config.logs.tbz2"
	if [ ${NUM_JOBS:-1} -eq 1 ] ; then
		echo " Last logfile entries:"
	else
		echo "Since the build was done in parallel, I'm not sure where the error"
		echo "happened.  The best thing to do is re-run the BuildToolChain script"
		echo "with the '-j 1' option.  The full log can be found here:"
		out=${ACTUAL_LOGFILE}
	fi
	echo "${out}"

	wait ${pid}
	) 1>&2

	exit 1
}

_log_it()
{
	# try and handle double and single quotes
	local fmt="$1"; shift
	local o
	o=`echo "$*" | sed -e 's:":\\\\":g'`
	eval printf "\"${fmt}\"" "\"$o\"" ${LOGFILE}
}
log_it()
{
	_log_it '###\n%s\n\n' "$@"
}
log_echo()
{
	_log_it '%s\n' "$@"
	echo "$*"
}
log_printf()
{
	log_echo "$(printf "$@")"
}

run_cmd_nodie()
{
	log_it "$@"
	# since logfile might contain redirection, we need to `eval`
	# since args might contain whitespace, we need to requote
	eval \($(requote "$@")\) ${LOGFILE} </dev/null
}
run_cmd()
{
	run_cmd_nodie "$@" || die_with_log
}

check_installed_files()
{
	local pkg=$1 ; shift
	local pfx=$1 ; shift
	local f
	for f in "$@" ; do
		f="${pfx}${f}${EXEEXT}"
		if [ ! -f "${f}" ] ; then
			die_with_log "${pkg} did not build properly : Missing ${f}"
		fi
	done
}

change_dir()
{
	log_it cd "$@"
	cd "$@" || die_with_log
}
clean_build_dir()
{
	change_dir "${DIR_BUILD}"
	${CLEAN_BUILD_DIRS} && run_cmd rm -rf "$@"
}
clean_dir()
{
	change_dir "${DIR_BUILD}"
	run_cmd rm -rf "$@"
}
change_clean_dir()
{
	clean_dir "$@"
	mkdir -p "$@"
	change_dir "$@"
}

echo_date()
{
	# if we can fit the msg on one line, then do it.  else,
	# try and split it on word boundaries.  if all else fails,
	# give it its own line.
	local recurse
	case $1 in
		-[rR]) recurse=$1; shift;;
	esac
	local date="$(date '+%d %b %Y %T')"
	local msg="$*"
	local bytes_for_msg=$((${COLUMNS:-80} - ${#date} - 5))

	if [ -n "${recurse}" ] || [ ${#msg} -le ${bytes_for_msg} ] ; then
		local banner="***" full
		if [ -n "${recurse}" ] ; then
			[ ${#msg} -eq 0 ] && return 1
			[ "${recurse}" = "-r" ] && banner="   " date=""
		fi
		[ ${#msg} -gt ${bytes_for_msg} ] && full=${msg} msg=""
		if [ -n "${msg}" ] || [ "${recurse}" = "-R" ] ; then
			printf "%s %-${bytes_for_msg}s %s\n" "${banner}" "${msg}" "${date}"
		fi
		[ -n "${full}" ] && echo "   " ${full}
	else
		local split word
		recurse="-R"
		for word in ${msg} ; do
			if [ $((${#split} + ${#word})) -ge ${bytes_for_msg} ] ; then
				echo_date ${recurse} ${split} && recurse="-r"
				split=""
			fi
			split="${split} ${word}"
		done
		echo_date ${recurse} ${split}
		recurse=""
	fi

	[ -z "${recurse}" ] && log_it "${msg}"
	return 0
}

get_scm_ver()
{
	if [ -n "$PKGVERSION" ] ; then
		echo "$PKGVERSION"
		return 0
	fi

	local src=$1 SCM_URL SCM_REV

	if [ -d "$src/.svn" ] ; then
		eval $(svn info "$src" 2>/dev/null | \
			sed -n \
				-e '/^URL:/s:.* .*/trunk/.*:SCM_URL=trunk:p' \
				-e '/^URL:/s:.* .*/branches/toolchain_\(.*\)_branch/.*:SCM_URL=\1:p' \
				-e '/^Last Changed Rev:/s:.* :SCM_REV=:p' \
		)
		[ -n "$SCM_REV" ] && SCM_REV="svn-${SCM_REV}"
	elif [ -d "$src/../.git" ] ; then
		eval $(cd "$src"; \
			SCM_URL=$(git branch | awk '$1 == "*" { sub(/^[*][[:space:]]*/, ""); gsub(/[()]/, ""); gsub(/[[:space:]]/, "-"); print; }')
			echo SCM_URL=${SCM_URL}
			echo SCM_REV=$(git rev-parse --short --verify HEAD)
		)
		[ -n "$SCM_REV" ] && SCM_REV="git-${SCM_REV}"
	fi
	[ -n "$SCM_URL" ] && SCM_URL="-${SCM_URL}"
	[ -n "$SCM_REV" ] && SCM_REV="/${SCM_REV}"
	echo "ADI${SCM_URL}${SCM_REV}"
}

# ised <file> <normal sed args>
ised()
{
	# not everyone supports the -i option, or does so correctly, so fake it
	[ -e "$1" ] || return 0
	sed "$@" > "$1".tmp
	mv "$1".tmp "$1"
}
