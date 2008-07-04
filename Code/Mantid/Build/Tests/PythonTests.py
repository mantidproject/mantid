import sys
import os
#Add Bin\Shared to Python path
sys.path.append('../../Bin/Shared/')
import libMantidPythonAPI

#Put tests here
def TestLoadRaw():
	file = "../../../../Test/Data/MAR11060.RAW"
	ws = libMantidPythonAPI.loadIsisRawFile(file, "MariWorkspace1")
	
	if ws != None:
		print "RESULT: TestLoadRaw PASSED"
		return "RESULT: TestLoadRaw PASSED"
	else:
		print "RESULT: TestLoadRaw FAILED"
		return "RESULT: TestLoadRaw FAILED"

def TestGetAlgorithmNames():
	algs = libMantidPythonAPI.getAlgorithmNames()
	
	if len(algs) > 0:
		print "RESULT: TestGetAlgorithmNames PASSED"
		return "RESULT: TestGetAlgorithmNames PASSED"
	else:
		print "RESULT: TestGetAlgorithmNames FAILED"
		return "RESULT: TestGetAlgorithmNames FAILED"
	
def TestGetWorkspaceNames():
	names = libMantidPythonAPI.getWorkspaceNames()
	
	if len(names) > 0:
		print "RESULT: TestGetWorkspaceNames PASSED"
		return "RESULT: TestGetWorkspaceNames PASSED"
	else:
		print "RESULT: TestGetWorkspaceNames FAILED"
		return "RESULT: TestGetWorkspaceNames FAILED"
	
def TestWorkspaceHistory():
	file = "../../../../Test/Data/HET15869.RAW"

	ws = libMantidPythonAPI.loadIsisRawFile(file, "HetWorkspace1")
	h = ws.getHistory()
	a = h.getAlgorithmHistories()
	p = a[0].getParameters()

	if p[0].value() == file:
		print "RESULT: TestWorkspaceHistory PASSED"
		return "RESULT: TestWorkspaceHistory PASSED"
	else:
		print "RESULT: TestWorkspaceHistory FAILED"
		return "RESULT: TestWorkspaceHistory FAILED"
	
def TestCreateFrameworkManager():
	mgr = libMantidPythonAPI.FrameworkManager()
	
	if mgr != None:
		print "RESULT: TestCreateFrameworkManager PASSED"
		return "RESULT: TestCreateFrameworkManager PASSED"
	else:
		print "RESULT: TestCreateFrameworkManager FAILED"
		return"RESULT: TestCreateFrameworkManager FAILED"
	
def TestCreateLoadRawAlgorithm():
	mgr = libMantidPythonAPI.FrameworkManager()
	
	alg = mgr.createAlgorithm("LoadRaw", -1)
		
	if alg != None:
		print "RESULT: TestCreateLoadRawAlgorithm PASSED"
		return "RESULT: TestCreateLoadRawAlgorithm PASSED"
	else:
		print "RESULT: TestCreateLoadRawAlgorithm FAILED"
		return "RESULT: TestCreateLoadRawAlgorithm FAILED"
	
def TestDeleteWorkspace():
	names = libMantidPythonAPI.getWorkspaceNames()
	
	count = len(names)
	
	if count == 0:
		print "RESULT: TestDeleteWorkspace FAILED"
		return "RESULT: TestDeleteWorkspace FAILED"
	
	mgr = libMantidPythonAPI.FrameworkManager()
	
	mgr.deleteWorkspace(names[0])
	
	names = libMantidPythonAPI.getWorkspaceNames()
		
	if len(names) == count -1:
		print "RESULT: TestDeleteWorkspace PASSED"
		return "RESULT: TestDeleteWorkspace PASSED"
	else:
		print "RESULT: TestDeleteWorkspace FAILED"
		return "RESULT: TestDeleteWorkspace FAILED"

#List tests to run here
results = []

results.append(TestLoadRaw())
results.append(TestGetAlgorithmNames())
results.append(TestGetWorkspaceNames())
results.append(TestWorkspaceHistory())
results.append(TestCreateFrameworkManager())
results.append(TestCreateLoadRawAlgorithm())
results.append(TestDeleteWorkspace())

f=open('../../../logs/PythonResults.log', 'w')
for res in results:
	f.write(res)
	
f.close
	

