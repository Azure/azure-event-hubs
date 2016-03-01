#!/bin/bash
#set -o pipefail
#

set -e

script_dir=$(cd "$(dirname "$0")" && pwd)
build_root=$(cd "${script_dir}/../.." && pwd)
log_dir=$build_root
run_e2e_tests=OFF
run_longhaul_tests=OFF
skip_unittests=OFF

usage ()
{
    echo "build.sh [options]"
    echo "options"
    echo " -cl, --compileoption <value>  specify a compile option to be passed to gcc"
    echo "   Example: -cl -O1 -cl ..."
    echo " --run-e2e-tests               runs the end-to-end tests (e2e tests are not run by default)"
    echo " --skip-unittests              skip the running of unit tests (unit tests are run by default)"
    exit 1
}

process_args ()
{
    save_next_arg=0
    extracloptions=" "

    for arg in $*
    do      
      if [ $save_next_arg == 1 ]
      then
        # save arg to pass to gcc
        extracloptions="$arg $extracloptions"
        save_next_arg=0
      else
          case "$arg" in
              "-cl" | "--compileoption" ) save_next_arg=1;;
              "--run-e2e-tests" ) run_e2e_tests=ON;;
              "--skip-unittests" ) skip_unittests=ON;;
              * ) usage;;
          esac
      fi
    done
}

process_args $*

rm -r -f ~/cmake
mkdir ~/cmake
pushd ~/cmake
cmake -DcompileOption_C:STRING="$extracloptions" -Drun_e2e_tests:BOOL=$run_e2e_tests -Drun_longhaul_tests=$run_longhaul_tests -Dskip_unittests:BOOL=$skip_unittests $build_root
make --jobs=$(nproc)
ctest -C "Debug" -V
popd