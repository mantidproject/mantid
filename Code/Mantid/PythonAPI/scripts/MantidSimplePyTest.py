setWorkingDirectory("../../../Test/Data/")

# Print the available algorithms
mtdHelp()

# Print information on a specific algorithm
mtdHelp("LoadRaw")

# Perform some algorithms
LoadRaw("HET15869.RAW","test")
ConvertUnits("test","converted","dSpacing")
Rebin("converted","rebinned","0.1,0.001,5")

# clear up intermediate workspaces
mtd.deleteWorkspace("test")
mtd.deleteWorkspace("converted")

# extract the one we want
w = mtd.getMatrixWorkspace('rebinned')

print "Rebinned workspace has " + str(w.getNumberHistograms()) + " histograms"
print "Rebinned workspace, spectrum 450's X data size " + str(w.readX(450).size()) + " bin boundaries"

