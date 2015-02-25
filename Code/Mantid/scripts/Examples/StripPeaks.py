#pylint: disable=invalid-name
# MantidPlot example script
#
# Using StripPeaks + dialog boxes
#

LoadRawDialog(OutputWorkspace="GEM40979")
alg= AlignDetectorsDialog("GEM40979",OutputWorkspace="aligned")
DeleteWorkspace("GEM40979")
calfile = alg.getPropertyValue("CalibrationFile")
DiffractionFocussing("aligned",calfile,OutputWorkspace="focussed")
DeleteWorkspace("aligned")
StripPeaks("focussed",OutputWorkspace="stripped")

# Plot a spectrum from each remaining workspace
g1 = plotSpectrum(["focussed","stripped"],5)

# Rescale the x-axis to show an interesting region
g1.activeLayer().setScale(Layer.Bottom,0,2.3)
print "Done!"
