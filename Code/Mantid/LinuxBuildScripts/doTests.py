import os

print os.getcwd()

f=open('../TestsToRun', 'r')

for test in f:
	name = test.split('/')
	os.popen('echo ' + name[-1].strip().rstrip('.h') + ' >> ../../../logs/testResults.log')
	os.popen('./' + name[-1].strip().rstrip('.h') + ' >> ../../../logs/testResults.log')
	os.popen('rm ./' + name[-1].strip().rstrip('.h') + '.cpp')
	

	
