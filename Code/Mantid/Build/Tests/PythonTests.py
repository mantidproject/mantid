import sys
#Add Bin\Shared to Python path
sys.path.append('../../Bin/Shared/')
import libMantidPythonAPI

#Put tests here
def TestLoadRaw():
	file = "../../../../Test/Data/MAR11060.RAW"
	ws = libMantidPythonAPI.loadIsisRawFile(file, "MariWorkspace1")
	
	if ws != None:
		print "RESULT: TestLoadRaw PASSED"
	else:
		print "RESULT: TestLoadRaw FAILED"
	
	return

def TestGetAlgorithmNames():
	algs = libMantidPythonAPI.getAlgorithmNames()
	
	if len(algs) > 0:
		print "RESULT: TestGetAlgorithmNames PASSED"
	else:
		print "RESULT: TestGetAlgorithmNames FAILED"
		
	return
	
def TestGetWorkspaceNames():
	names = libMantidPythonAPI.getWorkspaceNames()
	
	if len(names) > 0:
		print "RESULT: TestGetWorkspaceNames PASSED"
	else:
		print "RESULT: TestGetWorkspaceNames FAILED"
		
	return
	
def TestWorkspaceHistory():
	file = "../../../../Test/Data/HET15869.RAW"

	ws = libMantidPythonAPI.loadIsisRawFile(file, "HetWorkspace1")
	h = ws.getHistory()
	a = h.getAlgorithmHistories()
	p = a[0].getParameters()

	if p[0].value() == file:
		print "RESULT: TestWorkspaceHistory PASSED"
	else:
		print "RESULT: TestWorkspaceHistory FAILED"
	
	return
	
def TestCreateFrameworkManager():
	mgr = libMantidPythonAPI.FrameworkManager()
	
	if mgr != None:
		print "RESULT: TestCreateFrameworkManager PASSED"
	else:
		print "RESULT: TestCreateFrameworkManager FAILED"
		
	return
	
def TestCreateLoadRawAlgorithm():
	mgr = libMantidPythonAPI.FrameworkManager()
	
	alg = mgr.createAlgorithm("LoadRaw", -1)
		
	if alg != None:
		print "RESULT: TestCreateLoadRawAlgorithm PASSED"
	else:
		print "RESULT: TestCreateLoadRawAlgorithm FAILED"
		
	return
	
def TestDeleteWorkspace():
	names = libMantidPythonAPI.getWorkspaceNames()
	
	count = len(names)
	
	if count == 0:
		print "RESULT: TestDeleteWorkspace FAILED"
		return
	
	mgr = libMantidPythonAPI.FrameworkManager()
	
	mgr.deleteWorkspace(names[0])
	
	names = libMantidPythonAPI.getWorkspaceNames()
		
	if len(names) == count -1:
		print "RESULT: TestDeleteWorkspace PASSED"
	else:
		print "RESULT: TestDeleteWorkspace FAILED"
		
	return

#List tests to run here
TestLoadRaw()
TestGetAlgorithmNames()
TestGetWorkspaceNames()
TestWorkspaceHistory()
TestCreateFrameworkManager()
TestCreateLoadRawAlgorithm()
TestDeleteWorkspace()

