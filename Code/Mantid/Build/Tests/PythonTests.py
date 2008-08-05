import sys
import os
#Add Bin\Shared to Python path
sys.path.append('../../Bin/Shared/')
import libMantidPythonAPI

file = "../../../../Test/Data/MAR11060.RAW"

#Put tests here
def TestCreateFrameworkManager():
	mgr = libMantidPythonAPI.FrameworkManager()
	
	if mgr != None:
		print "RESULT: TestCreateFrameworkManager PASSED"
		return "RESULT: TestCreateFrameworkManager PASSED"
	else:
		print "RESULT: TestCreateFrameworkManager FAILED"
		return"RESULT: TestCreateFrameworkManager FAILED"

def TestLoadRaw():
	ws = libMantidPythonAPI.loadIsisRawFile(file, "MariWorkspace1")
	
	if ws != None:
		print "RESULT: TestLoadRaw PASSED"
		return "RESULT: TestLoadRaw PASSED"
	else:
		print "RESULT: TestLoadRaw FAILED"
		return "RESULT: TestLoadRaw FAILED"

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

def TestGetSpectraMap():
	ws = libMantidPythonAPI.loadIsisRawFile(file, "MariWorkspace1")
	sdm = ws.getSpectraMap()
	
	result = "";
	
	if sdm.ndet(0) == 0 and sdm.ndet(1) == 1:
		result =  "RESULT: TestGetSpectraMap PASSED"
	else:
		result = "RESULT: TestGetSpectraMap FAILED"
	
	mgr = libMantidPythonAPI.FrameworkManager()
	mgr.deleteWorkspace("MariWorkspace1")
	print result
	return result

def TestGetAlgorithmNames():
	algs = libMantidPythonAPI.getAlgorithmNames()
	
	if len(algs) > 0:
		print "RESULT: TestGetAlgorithmNames PASSED"
		return "RESULT: TestGetAlgorithmNames PASSED"
	else:
		print "RESULT: TestGetAlgorithmNames FAILED"
		return "RESULT: TestGetAlgorithmNames FAILED"
	
def TestGetWorkspaceNames():
	ws = libMantidPythonAPI.loadIsisRawFile(file, "MariWorkspace1")
	names = libMantidPythonAPI.getWorkspaceNames()
	
	result = "";
	
	if len(names) > 0:
		result =  "RESULT: TestGetWorkspaceNames PASSED"
	else:
		result = "RESULT: TestGetWorkspaceNames FAILED"

	mgr = libMantidPythonAPI.FrameworkManager()
	mgr.deleteWorkspace("MariWorkspace1")
	print result
	return result
	
def TestWorkspaceHistory():
	ws = libMantidPythonAPI.loadIsisRawFile(file, "MariWorkspace1")
	h = ws.getHistory()
	a = h.getAlgorithmHistories()
	p = a[0].getProperties()

	result = ""

	if p[0].value() == file:
		result =  "RESULT: TestWorkspaceHistory PASSED"
	else:
		result = "RESULT: TestWorkspaceHistory FAILED"
		
	mgr = libMantidPythonAPI.FrameworkManager()
	mgr.deleteWorkspace("MariWorkspace1")
	print result
	return result
	
def TestCreateLoadRawAlgorithm():
	mgr = libMantidPythonAPI.FrameworkManager()
	
	alg = mgr.createAlgorithm("LoadRaw")
		
	if alg != None:
		print "RESULT: TestCreateLoadRawAlgorithm PASSED"
		return "RESULT: TestCreateLoadRawAlgorithm PASSED"
	else:
		print "RESULT: TestCreateLoadRawAlgorithm FAILED"
		return "RESULT: TestCreateLoadRawAlgorithm FAILED"

def TestExecuteLoadRawAlgorithm():
	mgr = libMantidPythonAPI.FrameworkManager()
	
	alg = mgr.execute("LoadRaw", "OutputWorkspace=MariWorkspace1;Filename=" + file)
	
	result = ""
		
	if alg != None:
		result = "RESULT: TestExecuteLoadRawAlgorithm PASSED"
	else:
		result = "RESULT: TestExecuteLoadRawAlgorithm FAILED"
		
	mgr.deleteWorkspace("MariWorkspace1")
	print result
	return result

#List tests to run here
results = []

results.append(TestCreateFrameworkManager())
results.append(TestLoadRaw())
results.append(TestDeleteWorkspace())
results.append(TestGetSpectraMap())
results.append(TestGetAlgorithmNames())
results.append(TestGetWorkspaceNames())
results.append(TestWorkspaceHistory())
results.append(TestCreateLoadRawAlgorithm())
results.append(TestExecuteLoadRawAlgorithm())


f=open('../../../logs/PythonResults.log', 'w')
for res in results:
	f.write(res + '\n')
	
f.close
	

