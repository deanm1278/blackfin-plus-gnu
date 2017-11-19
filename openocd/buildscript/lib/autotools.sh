# give it the log file to rip out EXEEXT
get_exeext() { eval `grep ^EXEEXT= "$@"`; }

reset_autotool_hooks()
{
	at_clean() { :; }
	at_make_args() { :; }
	at_install_post() { :; }
}

touch_tree()
{
	# fix autotool timestamps
	log_it find "$@" -print0 ... xargs -0 touch -r "$1"
	find "$@" -print0 | xargs -0 touch -r "$1"
}

build_autotooled_pkg()
{
	local ver
	local src="$1"
	local pkg="${src##*/}"
	local build="${DIR_BUILD}/${pkg}_build"
	shift

	resume_check "${pkg}" && return 0

	change_dir "${src}"

	ver=$("${src}"/configure --version | awk '{print $NF;exit}')

	echo_date "${pkg}: cleaning (${ver})"
	touch_tree .
	at_clean
	[ -e Makefile ] && run_cmd $MAKE distclean

	change_clean_dir "${build}"

	echo_date "${pkg}: configuring"
	export ac_cv_path_DOXYGEN=:
	grep -qs disable-silent-rules "${src}"/configure && \
		set -- --disable-silent-rules "$@"
	! is_abs_dir "${src}" && src=../${src}
	run_cmd "${src}"/configure --prefix= $BUILD_TARGET $HOST_TARGET "$@"

	echo_date "${pkg}: building"
	run_cmd $MAKE $(at_make_args)
	get_exeext "${build}"/config.log

	echo_date "${pkg}: installing"
	at_install() {
		run_cmd $MAKE install DESTDIR="$1" $(at_make_args)
		run_cmd rm -f "$1"/lib/*.la
		run_cmd_nodie mv "$1"/include/* "${STAGEDIR}"/usr/include/
		run_cmd_nodie mv "$1"/lib/*.a "${STAGEDIR}"/usr/lib/
		run_cmd_nodie mv "$1"/lib/pkgconfig/* "${STAGEDIR}"/usr/lib/pkgconfig/
		run_cmd_nodie rmdir "$1"/lib/pkgconfig "$1"/lib "$1"/include
		at_install_post
	}
	at_install "${DIR_BUILD}/${CHOST}"

	# mung the local .pc files so other packages in here can find them
	local f
	for f in "${STAGEDIR}"/usr/lib/pkgconfig/*.pc ; do
		ised "${f}" -e "/^prefix=/s:=.*:=${STAGEDIR}/usr:"
	done

	clean_build_dir "${build}"

	resume_save

	reset_autotool_hooks
}

reset_autotool_hooks
