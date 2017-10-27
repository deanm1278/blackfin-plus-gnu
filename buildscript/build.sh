. "${0%/*}/lib/lib.sh" || exit 1

#check prereqs

PREREQ_FILE="autoconf automake awk /bin/sh bison cp echo flex gcc gettext grep ln m4 make makeinfo mkdir msgfmt mv rm sed tail wc which pkg-config"
DIR_ELF_OUTPUT=$(pwd)"/bfin-elf"
DIR_BUILD=$(pwd)
DIR_BINUTILS_SOURCE=$(pwd)"/../binutils-gdb"
DIR_GCC_SOURCE=$(pwd)"/../gcc"
DIR_LDR_UTILS_SOURCE=$(pwd)"/../ldr-utils"
MAKE="make -j10"

CBUILD= # the system we are compiling on
CHOST= # the system the final compiler will run on

CBUILD=$($DIR_APP/config.guess)
BUILD_TARGET="--build=${CBUILD}"
: ${CHOST:=${CBUILD}}
HOST_TARGET="--host=${CHOST}"

PWD=$(pwd)

# Build up a set of binutils
#  $1 - toolchain prefix (bfin-XXX-gcc)
#  $2 - configure --prefix=XXX
build_binutils()
{
	local target="bfin-$1"
	local prefix="$2"
	local pkg="binutils/gdb"

	echo "configuring binutils..."
	change_clean_dir binutils_build
	set -- \
		--prefix=${prefix} --target=${target} --without-newlib
	run_cmd "$DIR_BINUTILS_SOURCE"/configure $BUILD_TARGET $HOST_TARGET "$@"

	echo "building binutils..."
	run_cmd $MAKE all-binutils all-gas all-ld all-sim all-gdb

	echo "installing binutils..."
	run_cmd $MAKE install-binutils install-gas install-ld install-sim install-gdb

	cd DIR_BUILD
}

#build gcc
#  $1 - toolchain prefix (bfin-XXX-gcc)
#  $2 - configure --prefix=XXX
build_gcc()
{
	local target="bfin-$1"
	local prefix="$2"

	echo "configuring gcc..."
	change_clean_dir gcc_build
	#cd gcc_build

	run_cmd "${DIR_GCC_SOURCE}"/configure $BUILD_TARGET $HOST_TARGET --prefix=${prefix} --target=${target} \
		--disable-libstdcxx-pch \
		--enable-languages=c \
		--enable-clocale=generic \
		--disable-werror --with-newlib \
		--disable-symvers --disable-libssp --disable-libffi --disable-libgcj \
		--enable-version-specific-runtime-libs --enable-__cxa_atexit

	echo "building gcc..."
	run_cmd $MAKE

	echo "installing gcc..."
	run_cmd make install
}

build_ldr_utils()
{
	local x t d sf df
	build_autotooled_pkg "${DIR_LDR_UTILS_SOURCE}"

	for x in elf:${DIR_ELF_OUTPUT}; do
		t=${x%%:*}
		d=${x##*:}
		sf="${d}/bin/bfin-ldr$EXEEXT"
		df="${d}/bin/bfin-${t}-ldr$EXEEXT"
		[ -e "${sf}" ] || continue
		run_cmd ln -f "${sf}" "${df}"
	done
}


#execution
#check_prereqs_verbose ${PREREQ_FILE}

#mkdir binutils_build
#mkdir gcc_build
#mkdir $DIR_ELF_OUTPUT

#build elf toolchain

#build_binutils elf $DIR_ELF_OUTPUT
#build_gcc elf $DIR_ELF_OUTPUT

export STAGEDIR=${DIR_BUILD}/staging_build
mk_output_dir "staging" "${STAGEDIR}"

export PKG_CONFIG_PATH="${STAGEDIR}/usr/lib/pkgconfig${PKG_CONFIG_PATH:+:${PKG_CONFIG_PATH}}"
: ${PKG_CONFIG:=pkg-config --static}
export PKG_CONFIG

build_ldr_utils
