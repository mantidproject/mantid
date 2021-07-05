#!/bin/bash

function enable_coverage(){
    # Appends any coverage settings to the current CMake String passed as a param
    local EXISTING_OPTS="$1"

    if [[ ${JOB_NAME} == *coverage* ]]; then
        local COVERAGE=ON
        # Since we currently don't support passing a timeout, hardcode it for coverage jobs
        local TIMEOUT=1200  # Seconds, 4x normal 300 second job
    else
        local COVERAGE=OFF
        local TIMEOUT=300
    fi

    local OPTS="$EXISTING_OPTS -DCOVERAGE=${COVERAGE} -DTESTING_TIMEOUT=${TIMEOUT}"
    echo "$OPTS"
}



