import os

f=open('../TestsToRun', 'r')

g=open('TestsScript.bat', 'w')

logfile = '../../../../../../logs/Mantid/testResults.log'
g.write('set PATH=../../../Third_Party\lib\win32;%PATH%\n')
g.write('set PATH=../../Bin/Shared;%PATH%\n')

for test in f:
	name = test.split('/')
	g.write('echo ' + name[-1].strip().rstrip('.h') + ' >> ' + logfile + '\n')
	g.write(name[-1].strip().rstrip('.h') + ' >> ' + logfile + '\n')
	g.write('del ' + name[-1].strip().rstrip('.h') + '.cpp\n')
	
f.close()
g.close()
	
