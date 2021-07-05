#!/bin/bash

function enable_coverage(){
    # Appends any coverage settings to the current CMake String

    if [[ ${JOB_NAME} == *coverage* ]]; then
        local COVERAGE=ON
    else
        local COVERAGE=OFF
    fi


    local cmake_string="$1 -DCOVERAGE=${COVERAGE}"
    echo "$cmake_string"
}



