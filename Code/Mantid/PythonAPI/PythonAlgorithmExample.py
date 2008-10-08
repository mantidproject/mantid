import libMantidPythonAPI

mgr = libMantidPythonAPI.FrameworkManager()
algs = libMantidPythonAPI.getAlgorithmNames()
print len(algs)

class test(libMantidPythonAPI.PyAlgorithm):
	def PyInit(self):
		print 'Hello from the init of ' + self.name()
	def PyExec(self):
		print 'Hello from the exec of ' + self.name()
		
a = test("MyAlg123")

print mgr.addPythonAlgorithm(a)

mgr.executePythonAlgorithm("MyAlg123")


