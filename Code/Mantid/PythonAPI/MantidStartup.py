import MantidPythonAPI
print "Mantid Interactive Scripting Interface"
mantid = MantidPythonAPI.PythonInterface()
print "Mantid API object 'mantid' ready\n"

print 'To load a raw file execute:'
print '\tMantid.LoadIsisRawFile("MAR11060.RAW","TestWorkspace");'
#print mantid.GetAlgorithmNames()