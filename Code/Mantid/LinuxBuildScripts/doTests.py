import os

f=open('TestsToRun', 'r')

for test in f:
	name = test.split('/')
	os.popen('echo Tests/' + name[-1].strip().rstrip('.h') + ' >> ../logs/testResults.log')
	os.popen('./Tests/' + name[-1].strip().rstrip('.h') + ' >> ../logs/testResults.log')
	

	
