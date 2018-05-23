#!/bin/bash

target_arg="host"

while getopts "t:" opt; do
  case $opt in
    t ) target_arg="$OPTARG" ;;
    * ) echo "Usage: $0 [-t target]" 1>&2
        exit 1 ;;
  esac
done

if [ "${target_arg}" = "host" ]; then
    target="test"
elif [ "${target_arg}" = "csim" ]; then
    target="test_csim"
elif [ "${target_arg}" = "device" ]; then
    target="run"
elif [ "${target_arg}" != "clean" ]; then
    echo "Unknown target : $target_arg" 1>&2
    exit 1 ;
fi


cd src
dirs=$(echo $(ls -d */) | sed 's/\///g')

if [ "${target_arg}" = "clean" ]; then
    for dir in ${dirs}; do
        cd ${dir}
        make clean &> /dev/null
        rm -f *.log
        cd ../
    done
else
    echo -e "        TARGET : ${target_arg}\n"
    all_succeed=true
    for dir in ${dirs}; do
        echo -e "            Testing : ${dir}"
        cd ${dir}
        log=`make -B $target 2>&1`
        ret="$?"
        echo "$log" > make_${target}.log
        if [ $ret -eq 0 ]; then
            echo -e "             Result : \e[32mSUCCEED\e[m\n"
        else
            all_succeed=false
            echo -e "             Result : \e[31mFAILED\e[m\n"
        fi
        cd ../
    done

    if $all_succeed; then
        echo -e "\e[32mTest Successed!\e[m"
        exit 0
    else
        echo -e "\e[31mTest Failed...\e[m"
        exit 1
    fi
fi
