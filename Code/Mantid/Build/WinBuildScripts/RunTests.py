import os

print 'Current path == ',os.getcwd()
os.system('Build\Winbuildscripts\BuildTests.bat')
os.system('Build\Winbuildscripts\ExecTests.bat')
