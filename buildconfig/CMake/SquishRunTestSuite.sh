#!/bin/sh
echo "Starting"
echo "Configuring the squish server...$1 --config addAUT $2 $3"
$1 --config addAUT $2 $3
echo "Starting the squish server...$1 --daemon"
$1 --daemon

results="xmljunit,${7}"

echo "Running the test suite...$4 --testsuite $5 --resultdir $6 --reportgen ${results}"
$4 --testsuite $5 --resultdir $6 --reportgen ${results}
returnValue=$?

echo "Stopping the squish server...$1 --stop"
$1 --stop

exit $returnValue
