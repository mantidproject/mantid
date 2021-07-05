#!/bin/bash

function enable_static_analysis(){


    if [[ ${JOB_NAME} == *address* ]]; then
        local SANITIZER="Address"

    elif [[ ${JOB_NAME} == *memory* ]]; then
        local SANITIZER="memory"

    elif [[ ${JOB_NAME} == *thread* ]]; then
        local SANITIZER="thread"

    elif [[ ${JOB_NAME} == *undefined* ]]; then
        local SANITIZER="undefined"
    fi

    if [[ -n $SANITIZER ]]; then
        OPTS="-DUSE_SANITIZER=${SANITIZER}"
    else
        OPTS=""
    fi
    echo $OPTS
}