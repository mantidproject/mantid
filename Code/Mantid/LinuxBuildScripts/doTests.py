import os

f=open('../TestsToRun', 'r')

g=open('TestsScript.sh', 'w')

for test in f:
	name = test.split('/')
	g.write('echo ' + name[-1].strip().rstrip('.h') + ' >> ../../../logs/testResults.log\n')
	g.write('./' + name[-1].strip().rstrip('.h') + ' >> ../../../logs/testResults.log\n')
	g.write('rm ./' + name[-1].strip().rstrip('.h') + '.cpp\n')
	
f.close()
g.close()
	
