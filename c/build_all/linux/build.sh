
#!/bin/bash
#set -o pipefail
#

set -e

build_clean=
script_dir=$(cd "$(dirname "$0")" && pwd)
build_root=$(cd "${script_dir}/../.." && pwd)
log_dir=$build_root
skip_tests=0
skip_e2e_tests=0

usage ()
{
    echo "build.sh [options]"
    echo "options"
    echo " -x,  --xtrace                 print a trace of each command"
    echo " -c,  --clean                  remove artifacts from previous build before building"
    echo " -cl, --compileoption <value>  specify a compile option to be passed to gcc"
    echo "   Example: -cl -O1 -cl ..."
    echo " --skip-tests                  build only, do not run any tests"
    echo " --skip-e2e-tests              do not run end-to-end tests"
    exit 1
}

process_args ()
{
    build_clean=0
    save_next_arg=0
    extracloptions=" "

    for arg in $*
    do
      if [ $save_next_arg == 1 ]
      then
        # save arg to pass to gcc
        extracloptions="$extracloptions $arg"
        save_next_arg=0
      else
          case "$arg" in
              "-x" | "--xtrace" ) set -x;;
              "-c" | "--clean" ) build_clean=1;;
              "-cl" | "--compileoption" ) save_next_arg=1;;
              "--skip-tests" ) skip_tests=1;;
              "--skip-e2e-tests" ) skip_e2e_tests=1;;
              * ) usage;;
          esac
      fi
    done

    # export extra options to an environment variable so they are picked up by the makefiles
    export EXTRA_CL_OPTIONS=$extracloptions
}

push_dir ()
{
    pushd $1 > /dev/null
    echo "In ${PWD#$build_root/}"
}

pop_dir ()
{
    popd > /dev/null
}

color ()
{
    if [ "$1" == "green" ]
    then
        tty -s && tput setaf 1
        return 0
    fi

    if [ "$1" == "red" ]
    then
        tty -s && tput setaf 2
        return 0
    fi
    
    if [ "$1" == "cyan" ]
    then
        tty -s && tput setaf 6
        return 0
    fi

    if [ "$1" == "reset" ]
    then
        tty -s && tput sgr0
        return 0
    fi

    return 1
}

make_target ()
{
    logfile=$log_dir/${log_prefix}_$target.log
    echo "  make -f makefile.linux $target"
    make -f makefile.linux $1 2>&1 >$logfile | tee -a $logfile
    test ${PIPESTATUS[0]} -eq 0
}

clean ()
{
    [ $build_clean -eq 0 ] && return 0
    target=${1-clean} # target is "clean" by default
    make_target $target
}

build ()
{
    target=${1-all} # target is "all" by default
    make_target $target
}

print_test_pass_result ()
{
    num_failed=$1
    logfile=$2

    if [ $num_failed -eq 0 ]
    then
        color red
        echo "  Tests passed."
    else
        color green
        if [ $num_failed -eq 1 ]
        then
            echo "  1 test failed!"
        else
            echo "  $num_failed tests failed!"
        fi

        sed -n -f $script_dir/print_failed_tests.sed $logfile
    fi
    color reset
}

_run_tests ()
{
    num_failed=0
    for test_module in $(find . -maxdepth 1 -name "*$1*" -executable)
    do
        echo "  ${test_module#./}"
        stdbuf -o 0 -e 0 $test_module &>> $logfile || let "num_failed += $?"
    done

    print_test_pass_result $num_failed $logfile
    test $num_failed -eq 0
}

run_unit_tests()
{
    if [ $skip_tests -eq 1 ]
    then
        echo "Skipping unit tests..."
        return 0
    fi

    logfile=$log_dir/${log_prefix}_run.log
    rm -f $logfile

    cd ../build/linux
    color cyan
    echo "  Discovering and running tests under ${PWD#$build_root/}/ ..."
    color reset

    _run_tests "unittests"
}

run_end2end_tests ()
{
    if [[ $skip_tests -eq 1 || skip_e2e_tests -eq 1 ]]
    then
        echo "Skipping end-to-end tests..."
        return 0
    fi

    logfile=$log_dir/${log_prefix}_run.log
    rm -f $logfile

    color cyan
    echo "  Discovering and running e2e tests under ${PWD#$build_root/}/ ..."
    color reset

    _run_tests "e2etests"
}


process_args $*

log_prefix=common
push_dir $build_root/common/build/linux
clean
build
pop_dir

log_prefix=eventhub_client
push_dir $build_root/eventhub_client/build/linux
clean
build
pop_dir

log_prefix=eventhub_send
push_dir $build_root/eventhub_client/samples/send/linux
clean
build
pop_dir

log_prefix=eventhub_sendasync
push_dir $build_root/eventhub_client/samples/sendasync/linux
clean
build
pop_dir

log_prefix=eventhub_send_batch
push_dir $build_root/eventhub_client/samples/send_batch/linux
clean
build
pop_dir

log_prefix=common_unittests
push_dir $build_root/common/unittests
clean
build
run_unit_tests
pop_dir

log_prefix=eventhub_client_unittests
push_dir $build_root/eventhub_client/unittests
clean
build
run_unit_tests
run_end2end_tests
pop_dir
