import os

f=open('Build/TestsToRun', 'r')

for test in f:
	name = test.split('/')
	os.popen('echo ' + name[-1].strip().rstrip('.h') + ' >> ../logs/testResults.log')
	os.popen('./Build/Tests/' + name[-1].strip().rstrip('.h') + ' >> ../logs/testResults.log')
	os.popen('rm ./Build/Tests/' + name[-1].strip().rstrip('.h') + '.*')
	

	
