import os

f=open('../TestsToRun', 'r')

g=open('TestsScript.sh', 'w')

g.write('#!/bin/sh\n')
g.write('LOGFILE=../../../../../../logs/testResults.log\n')
g.write('export LD_LIBRARY_PATH=../../Bin/Shared:$LD_LIBRARY_PATH\n')

for test in f:
	name = test.split('/')
	g.write('echo ' + name[-1].strip().rstrip('.h') + ' >> $LOGFILE\n')
	g.write('./' + name[-1].strip().rstrip('.h') + ' >> $LOGFILE\n')
	g.write('rm -f ./' + name[-1].strip().rstrip('.h') + '.cpp\n')
	
f.close()
g.close()
	
