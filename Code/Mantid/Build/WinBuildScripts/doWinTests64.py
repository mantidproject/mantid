import os

f=open('../TestsToRun', 'r')

g=open('TestsScript.bat', 'w')

#tests not to run
h=open('../TestsNotToRun64', 'r')
notRuns = []
for file in h:
	(filepath, filename) = os.path.split(file)
	notRuns.append(filename.strip())


logfile = '../../../../../../logs/Mantid/testResults.log'
g.write('set PATH=../../../Third_Party\lib\win64;%PATH%\n')
g.write('set PATH=../../Bin/Shared;%PATH%\n')

for test in f:
	name = test.split('/')
	
	try:
		notRuns.index(name[-1].strip())
		print "Not Running: ", name[-1].strip()
	except ValueError:
		g.write('echo ' + name[-1].strip().rstrip('.h') + ' >> ' + logfile + '\n')
		g.write(name[-1].strip().rstrip('.h') + ' >> ' + logfile + '\n')
		g.write('del ' + name[-1].strip().rstrip('.h') + '.cpp\n')
	
	
f.close()
g.close()
	
