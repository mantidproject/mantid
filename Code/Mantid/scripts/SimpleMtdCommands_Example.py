#
# Example: Basic Mantid commands
#

# Print the available algorithms
mtdHelp()

# Print information on a specific algorithm
mtdHelp(LoadRaw)

# Perform some algorithms
LoadRaw("HET15869.raw","test")
ConvertUnits("test","converted","dSpacing")
Rebin("converted","rebinned","0.1,0.001,5")

# clear up intermediate workspaces
mtd.deleteWorkspace("test")
mtd.deleteWorkspace("converted")

# extract the one we want
wksp = mtd['rebinned']

print "Rebinned workspace has " + str(wksp.getNumberHistograms()) + " histograms"
print "Spectrum 450's X data size = " + str(len(wksp.readX(450))) + " bin boundaries"
