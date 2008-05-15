import os

print 'Current path == ',os.getcwd()
os.system('Build\\build.bat 1> ../../../../logs/qtiplot/build.log 2> ../../../../logs/qtiplot/error.log')
