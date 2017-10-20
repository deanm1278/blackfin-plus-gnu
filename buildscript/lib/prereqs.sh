#
# All functions related to perquisites or toolchain checks live here
#

check_header() {
	local header=$1
	printf "#include <$header>\n" | gcc -E - >/dev/null
}

check_lib() {
	local lib=$1 prefix=$2 Lpaths temp_file
	# Old Linux installations keep X in /usr/X11R6.
	temp_file=$(mktemp || echo "${TMPDIR-/tmp}/conftest.o")
	[ -z "${prefix}" ] && Lpaths="-L/usr/X11R6/lib -L/sw/lib" || prefix="${prefix}-"
	echo "int main(){}" | ${prefix}gcc -x c - ${LDFLAGS} ${Lpaths} -l${lib} -o ${temp_file} >/dev/null
	local ret=$?
	rm -f "${temp_file}"
	return ${ret}
}

# check_group(package name, header list, library list)
check_source_group() {
	local pkg=$1
	local h headers=$2
	local l libs=$3
	local fail=0

	for h in ${headers} ; do
		check_header ${h} || fail=1
	done
	for l in ${libs} ; do
		check_lib ${l} || fail=1
	done

	if [ -n "${pkg}" ] && [ ${fail} -eq 1 ] ; then
		cat <<-EOF 1>&2

		You need to install some packages before building the toolchain.

		Your system lacks the development packages for:
		    ${pkg}

		The headers you need:
		    ${headers}

		The libs you need:
		    ${libs}

		EOF
		exit 1
	fi

	return ${fail}
}

check_source_packages() {
	echo "Checking for development packages (skip checks with the -D option)"
	check_source_group "zlib" "zlib.h" "z"
	check_source_group "curses" "ncurses.h" "ncurses"
	if ! check_source_group "" "X11/Xlib.h" "X11" ; then
		echo "X development packages were not found, disabling gui packages such as insight" 1>&2
		WITHOUT_X=true
	fi
}

check_build_env() {
	# These checks matter for cross-compiler bits only
	if [ $SKIP_ELF ] && [ ! $KERNEL_SOURCE ] && [ $SKIP_TARGET_LIBS ] ; then
		return 0
	fi

	# Check to make sure shell env vars are empty
	for flags in CFLAGS LDFLAGS CPPFLAGS CXXFLAGS ; do
		tmp=$(eval echo \$${flags})
		if [ -n "${tmp}" ] ; then
			printf "\nSince the shell $flags is set, it will override the choices made by configure.\nThis is normally bad, and will cause problems.\n\n"
			exit 1
		fi
	done
}

detect_version()
{
	local flag ver prog p
	prog=$1
	p=${2:-${prog##*/}}
	for flag in --version -version -V ; do
		ver=`${prog} ${flag} </dev/null 2>&1 | grep -ie "\<${p}\>" | grep -vi -e option -e Usage -e -v`
		if [ -n "${ver}" ] ; then
			echo "${ver}"
			return 0
		fi
	done
	return 1
}

check_prereqs_verbose()
{
	echo "Check http://gcc.gnu.org/install/prerequisites.html for more information"
	for file in "$@" ; do
		RUN=`which -a $file 2>/dev/null| wc -w`
		if [ $RUN -eq 0 ] ; then
			echo "!! $file: could not be found"
			for tmp in "$@" ; do
				if [ "$file" = "$tmp" ]; then
					echo "!! you need to install package which includes ${file} before building the toolchain will be sucessful"
				fi
			done
		else
			RUN=`type ${file##*/} | grep "shell builtin" | wc -l`
			if [ $RUN -eq 1 ] ; then
				echo "   $file seems to be a shell builtin"
				continue
			fi
			tmp=`detect_version "${file}"`
			if [ -n "$tmp" ] ; then
				echo "  " $tmp
				continue
			fi

			VER_ok=""
			for VER in --version -version -V
			do
				if tmp=`$file $VER < /dev/null 2>&1` ; then
					VER_ok=$tmp
				fi
				tmp=$(echo "$tmp" | grep -ie "version " | grep -vi -e "option" -e "Usage" -e "\-v")
				if [ -n "$tmp" ] ; then
					echo "  " $tmp
					break
				fi
			done
			if [ -n "$tmp" ] ; then
				continue
			fi
			file=$(which $file)
			echo "** Looking up file $file for more info${VER_ok:+ ($VER_ok)}"
			while [ -L "$file" ] ; do
				printf " * "
				file $file
				lfile=$(readlink $file)
				[ "${lfile#/}" = "${lfile}" ] \
					&& file="${file%/*}/${lfile}" \
					|| file="${lfile}"
			done
			printf " * "
			file "${file}"
		fi
	done
	echo "   SHELL = $SHELL"
	echo "Done checking for prerequisites"

	check_build_env
}

check_prereqs_short() {
	for PREREQ in "$@" ; do
		RUN=`which -a $PREREQ | wc -w`
		if [ $RUN -eq 0 ] ; then
			echo "Cannot find $PREREQ"
			exit 1
		elif [ $RUN -gt 1 ] ; then
			printf "Found multiple versions of $PREREQ, using the one at "
			which $PREREQ
		fi
	done

	check_build_env
}

check_fs_case() {
	local case
	rm -rf .case.test .CASE.test
	echo case > .case.test
	case=$(cat .CASE.test 2>/dev/null)
	if [ "$case" = "case" ] ; then
		error "you must use a case sensitive filesystem"
	fi
	rm -rf .case.test .CASE.test
}

scrub_path()
{
	##### Check to make sure a old version of bfin-elf is not here
	##### Unless we are cross-compiling our toolchain, then we need the
	##### old toolchain in our PATH ...
	ORIG_PATH=$PATH
	NEW_PATH=""
	local FOUND=0 RUN p
	for p in `echo $PATH | sed 's/:/ /g'` ; do
		[ -d "$p" ] || continue

		RUN=`find "$p/" -maxdepth 1 -name bfin-elf-gcc -o -name bfin-uclinux-gcc -o -name bfin-linux-uclibc-gcc`
		if [ -n "${RUN}" ] ; then
			FOUND=1
			if [ -z "$CHOST" ] ; then
				echo "Removing from PATH: $p"
			fi
		else
			NEW_PATH=${NEW_PATH:+${NEW_PATH}:}$p
		fi
	done
	if [ -z "$CHOST" ] ; then
		PATH=$NEW_PATH
	elif [ "$FOUND" = "0" ] ; then
		error "You need an existing Blackfin cross-compiler\n" \
		      " in order to cross-compile a cross-compiler."
	else
		NEW_PATH=$ORIG_PATH
	fi
}
