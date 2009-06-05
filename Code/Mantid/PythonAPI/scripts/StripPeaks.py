# Perform some algorithms
LoadRawDialog(OutputWorkspace="GEM40979")
alg = AlignDetectorsDialog("GEM40979","aligned")
mtd.deleteWorkspace("GEM40979")
calfile = alg.getPropertyValue("CalibrationFile")
DiffractionFocussingDialog("aligned","focussed",calfile)
mtd.deleteWorkspace("aligned")
StripPeaks("focussed","stripped")

# Plot a spectrum from each remaining workspace and merge
g1 = plotSpectrum("stripped",3)
mergePlots(g1, plotSpectrum("focussed",3))

# Plot a spectrum from each remaining workspace
g2 = plotSpectrum("stripped",5)
mergePlots(g2, plotSpectrum("focussed",5))

# Rescale the x-axis to show an interesting region
g1.activeLayer().setScale(Layer.Bottom,0,2.3)
g2.activeLayer().setScale(Layer.Bottom,0,2.3)

print "Done!"
