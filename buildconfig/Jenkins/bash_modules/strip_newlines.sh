#!/bin/bash

function strip_newlines(){
    # Strips whitespace from a given input
    # Adapted from https://stackoverflow.com/a/19345966
    echo $1|tr -d '\n'
}