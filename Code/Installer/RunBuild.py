import os

print 'Current path == ',os.getcwd()
os.system('build.bat 1> ../../../../logs/Installer/build.log 2> ../../../../logs/Installer/error.log')
