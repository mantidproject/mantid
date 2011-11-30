echo 'Starting the squish server...'
start %1

echo 'Running the test suite...'
%2 --testsuite %3 --reportgen xmljunit,%4
set result=%ERRORLEVEL%

echo 'Stopping the squish server...'
%1 --stop

exit \b %result%
