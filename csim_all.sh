#!/bin/bash

all_succeed=true

cd src
dirs=$(echo $(ls -d */) | sed 's/\///g')

for dir in ${dirs}
do
    echo -e "            Testing : ${dir}"
    cd ${dir}
    log=`make -B run_${dir}_test_csim 2>&1`
    if [ "$?" -eq 0 ]; then
        echo -e "             Result : \e[32mSUCCEED\e[m\n"
    else
        all_succeed=false
        echo -e "             Result : \e[31mFAILED\e[m"
        echo "${log}"
        echo ""
    fi
    cd ../
done

if $all_succeed; then
    echo -e "\e[32mTest Successed!\e[m"
else
    echo -e "\e[31mTest Failed...\e[m"
fi
