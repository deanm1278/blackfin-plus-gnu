#!/bin/bash

set -e -x

# force consistent sorting in generated files
# so we dont get pointless changes across builds
export LC_ALL=C

make distclean

# figure out which vcs we're using
if svn info >&/dev/null ; then
	VCS="svn"
elif git rev-parse HEAD >&/dev/null ; then
	VCS="git"
else
	echo "ERROR: no idea what vcs i'm using"
	exit 1
fi

# prep files for autotoolization
case ${VCS} in
svn)
	svn log > ChangeLog
	;;
git)
	if git config svn-remote.svn.url >/dev/null ; then
		git svn log > ChangeLog
	else
		git log > ChangeLog
	fi
	;;
esac
topfiles=$(echo *.c *.h)
sed -i "/^ldr_SOURCES/s:=.*:= ${topfiles} \$(RC_SOURCES):" Makefile.am
ver=$(./local-version.sh ${VCS})
sed -i "/^AC_INIT/s:\([^,]*,\)[^,]*:\1 [${ver}]:" configure.ac
testatfiles=$(cd tests; echo *.at)
testfiles=$(cd tests; echo *.c *.in elfs/* ldrs/*)
sed -i \
	-e "/^EXTRA_DIST/s:=.*:= ${testfiles} \$(AT_FILES) \$(TESTSUITE):" \
	-e "/^AT_FILES/s:=.*:= ${testatfiles}:" \
	tests/Makefile.am

find gnulib/{lib,m4}/ '(' -type f -print ')' -o '(' -name .svn -prune ')' | xargs -r rm -f
PATH=/usr/local/src/gnu/gnulib:${PATH}
gnulib-tool --source-base=gnulib/lib --m4-base=gnulib/m4 --import $(<gnulib/modules) || :
rm -f gnulib/*/.gitignore
find gnulib -name '*~' -exec rm {} +

autoreconf -f -i -v

# stupid automake bug
case ${VCS} in
svn) svn revert INSTALL ;;
git) git checkout INSTALL ;;
esac

# update copyrights automatically
case ${VCS} in
svn)
	for f in $(grep -lI 'Copyright.*Analog Devices Inc.' `svn ls`) ; do
		year=$(svn info $f | awk '$0 ~ /^Last Changed Date:/ {print $4}' | cut -d- -f1)
		sed -i \
			-e "s:\(Copyright\) [-0-9]* \(Analog Devices Inc.\):\1 2006-${year} \2:" \
			${f}
	done
	;;
esac

# test building
if [ -d build ] ; then
	chmod -R 777 build
	rm -rf build
fi
mkdir build
cd build
../configure
make
make check
make dist
make distcheck
