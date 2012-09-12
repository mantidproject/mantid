#!/bin/bash

#
#process_run.sh 
#

echo "Calling post_process.sh "$1

nohup /usr/bin/post_process.sh $1 >> /var/log/SNS_applications/autoreduce.log 2>&1 &

echo "End calling post_process.sh"
