# Perform some algorithms
LoadRawDialog(OutputWorkspace="GEM40979")
alg = AlignDetectorsDialog("GEM40979","aligned")
mtd.deleteWorkspace("GEM40979")
calfile = alg.getPropertyValue("CalibrationFile")
DiffractionFocussingDialog("aligned","focussed",calfile)
mtd.deleteWorkspace("aligned")
StripPeaks("focussed","stripped")

# Plot a spectrum from each remaining workspace
g1 = plotSpectrum("stripped",3)
g2 = plotSpectrum("focussed",3)

# Insert the first curve from the active graph in g2 into active graph in g1
g1.insertCurve(g2,0)
g1.activeLayer().setScale(Layer.Bottom,0,2.3)
g2.hide()

# Plot a spectrum from each remaining workspace
g3 = plotSpectrum("stripped",5)
g4 = plotSpectrum("focussed",5)

# Insert the first curve from the active graph in g2 into active graph in g1
g3.insertCurve(g4,0)
g3.activeLayer().setScale(Layer.Bottom,0,2.3)
g4.hide()

print "Done!"
