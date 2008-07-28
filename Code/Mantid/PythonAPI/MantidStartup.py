import MantidPythonAPI
print "Mantid Interactive Scripting Interface"
mtd = MantidPythonAPI.FrameworkManager()
print ""
print "Mantid Framework Manager 'mtd' ready"
print "Available Algorithms"
algNames= MantidPythonAPI.getAlgorithmNames()
for i in range(len(algNames)):
	print "\t" + algNames[i]
print 'To load a raw file execute:'
print '\tmtd.execute("LoadRaw","Data/MAR11060.RAW;TestWorkspace",-1)'