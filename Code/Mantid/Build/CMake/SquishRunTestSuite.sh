#!/bin/sh
echo "Starting"
echo "Starting the squish server...$1 --daemon"
$1 --daemon

results="xmljunit,${5}"

echo "Running the test suite...$2 --testsuite $3 --resultdir $4 --reportgen ${results}"
$2 --testsuite $3 --resultdir $4 --reportgen ${results}
returnValue=$?

echo "Stopping the squish server...$1 --stop"
$1 --stop

exit $returnValue
