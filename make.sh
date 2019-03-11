#!/bin/bash
# shellcheck disable=SC2164,SC2012

usage()
{
	echo >&2 "
	${0##*/} [rw|rw-debug|ro|ro-debug] [options]

	Avaliable options:

	-c PATH - Path to compiler 'bin' directory (MinGW only)
	-b DIR  - Build directory (default: .build)
	-h      - Show this help and exit
	-v      - Verbose build
	-n      - No build, only generate makefiles
	"

	exit 2
}

fail()
{
	echo >&2 "$0: $*"
	exit 1
}

trap 'fail "Error at $LINENO: $BASH_COMMAND"' ERR
set -E

optfunc()
{
	while getopts c:b:hvn opt; do
		case "$opt" in
			c)
				compiler="$OPTARG"
				;;
			b)
				bdir="$OPTARG"
				;;
			h)
				usage
				;;
			v)
				verbose='-v'
				;;
			n)
				no_build=1
				;;
			?)
				fail "Bad option $opt"
				;;
		esac
	done
}

bdir='.build'

while [ $# -ne 0 ]; do
	if [[ "$1" == -* ]]; then
		optfunc "$@"
		shift $(( OPTIND - 1 ))
		OPTIND=1
	else
		if [ -z "$btype" ]; then
			btype="$1"
			shift
		else
			echo >&2 "$0: Extra arguments: $*"
			usage
		fi
	fi
done

case "$btype" in
	ro-debug)
		cmake_args='-DAPFSUTIL_FORCE_RO:BOOL=true -DCMAKE_BUILD_TYPE=Debug'
		btype_str='RO Debug'
		;;
	ro|ro-release)
		cmake_args='-DAPFSUTIL_FORCE_RO:BOOL=true'
		btype_str='RO Release'
		;;
	debug|rw-debug)
		cmake_args='-DCMAKE_BUILD_TYPE=Debug'
		btype_str='RW/RO Debug'
		;;
	release|rw|rw-release|'')
		btype_str='RW/RO Release'
		;;
	*)
		echo >&2 "$0: Bad build type $btype"
		usage
		;;
esac

echo "Build type: $btype_str"

rm -rf "$bdir"
mkdir  "$bdir"
cd     "$bdir"

case "$(uname)" in
	CYGWIN*)
		echo 'Pure CYGWIN build is not supported, using MinGW instead'

		if [ -n "$compiler" ]; then
			test -d "$compiler"
			mingw_path=$(cygpath -w "$compiler")
		else
			mingw_path="$(ls -d /cygdrive/c/Program\ Files/mingw-w64/*/mingw64/bin 2>/dev/null | head -n 1)"
			if [ -d "$mingw_path" ]; then
				mingw_path=$(cygpath -w "$mingw_path")
			else
				mingw_path='C:\MinGW\bin'
			fi
		fi

		echo 'set PATH=C:\Program Files\CMake\bin;'"$mingw_path"$'\r' > build.bat
		echo $'cmake .. -G "MinGW Makefiles" -DCMAKE_EXE_LINKER_FLAGS="-static"\r' >> build.bat

		test -n "$no_build" || echo "cmake --build . $verbose"$'\r' >> build.bat
		cmd.exe /c build.bat
		;;
	MINGW32*)
		echo 'Assuming MSYS environment'

		cmake .. -G "MSYS Makefiles"
		test -n "$no_build" || cmake --build . $verbose
		;;
	*)
		cmake $cmake_args ..
		test -n "$no_build" || cmake --build . ${verbose:+-- VERBOSE=1}
		;;
esac

