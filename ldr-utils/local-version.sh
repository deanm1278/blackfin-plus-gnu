#!/bin/sh

case $1 in
svn)
	scm=svn
	ver=$(svn info ${0%/*} | sed -n '/^Last Changed Rev/s|^.*: ||p')
	;;
git)
	if git config svn-remote.svn.url >/dev/null ; then
		scm=svn
		ver=$(git svn info ${0%/*} | sed -n '/^Last Changed Rev/s|^.*: ||p')
	else
		scm=git
		ver=$(git rev-parse --short --verify HEAD)
	fi
	;;
*)
	echo "Usage: $0 <svn|git>" 1>&2
	scm=idk
	ver=IDK
	;;
esac

printf "%s-%s" "${scm}" "${ver}"
