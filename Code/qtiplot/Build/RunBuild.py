import os

print 'Current path == ',os.getcwd()
os.system('svn update')
os.system('build.bat 1> ../../../../logs/qtiplot/build.log 2> ../../../../logs/qtiplot/error.log')
