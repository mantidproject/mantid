#!/bin/sh
echo "Starting"
echo "Starting the squish server...$1 --daemon --config addAUT $2 $3"
$1 --daemon --config addAUT $2 $3

results="xmljunit,${7}"

echo "Running the test suite...$4 --testsuite $5 --resultdir $6 --reportgen ${results}"
$4 --testsuite $5 --resultdir $6 --reportgen ${results}
returnValue=$?

echo "Stopping the squish server...$1 --stop"
$1 --stop

exit $returnValue
