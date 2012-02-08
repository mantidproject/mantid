echo 'Configuring the squish server...'
%1 --config addAUT %2 %3
echo 'Starting the squish server...'
start %

echo 'Running the test suite...'
%4 --testsuite %5 --resultdir %6 --reportgen xmljunit,%7
set result=%ERRORLEVEL%

echo 'Stopping the squish server...'
%1 --stop

exit \b %result%
