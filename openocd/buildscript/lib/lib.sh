if [ -z "${_BUILDTOOLCHAIN_SETUP_}" ] ; then

APP_NAME=${0##*/}
DIR_APP=$(cd "${0%/*}" && pwd)
DIR_BOARDS=${DIR_APP}/boards

# Assume default toolchain dir is up one directory from the build script helper
DIR_SOURCE=..

_LIB_DIR=${DIR_APP}/lib
. "${_LIB_DIR}/funcs.sh"
. "${_LIB_DIR}/autotools.sh"
. "${_LIB_DIR}/prereqs.sh"
. "${_LIB_DIR}/resume.sh"
. "${_LIB_DIR}/bash.sh"

_BUILDTOOLCHAIN_SETUP_=1

fi
