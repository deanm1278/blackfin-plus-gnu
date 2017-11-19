#
# bash-specific constructs can't even be in the same file
#

if [ -n "${BASH_VERSION}" ] && shopt -s extdebug 2>/dev/null ; then
. "${_LIB_DIR}/_bash.sh"
else
dump_trace() { :; }
fi
