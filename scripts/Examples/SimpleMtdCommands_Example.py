#
# Example: Basic Mantid commands
#

import mantid.simpleapi as mantid

# Print the available algorithms
mantid.mtdHelp()

# Print information on a specific algorithm
mantid.mtdHelp(mantid.LoadRaw)

# Perform some algorithms
mantid.LoadRaw("HET15869.raw", OutputWorkspace="test")
mantid.ConvertUnits("test", "dSpacing", OutputWorkspace="converted")
mantid.Rebin("converted", "0.1,0.001,5", OutputWorkspace="rebinned")

# clear up intermediate workspaces
mantid.DeleteWorkspace("test")
mantid.DeleteWorkspace("converted")

# extract the one we want
wksp = mantid.mtd['rebinned']

print("Rebinned workspace has " + str(wksp.getNumberHistograms()) + " histograms")
print("Spectrum 450's X data size = " + str(len(wksp.readX(450))) + " bin boundaries")
