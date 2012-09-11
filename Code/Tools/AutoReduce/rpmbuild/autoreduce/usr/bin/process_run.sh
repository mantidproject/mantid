#!/bin/bash

#
#process_run.sh 
#

echo "calling post_process.sh"

nohup /usr/bin/post_process.sh $1 >> /var/log/SNS_applications/autoreduce.log 2>&1 &

echo "end calling post_process.sh"
