# Perform some algorithms
LoadRaw("c:/mantid/Test/Data/GEM40979.raw","GEM40979")
AlignDetectors("GEM40979","aligned","c:/mantid/Test/Data/offsets_2006_cycle064.cal")
mtd.deleteWorkspace("GEM40979")
DiffractionFocussing("aligned","focussed","c:/mantid/Test/Data/offsets_2006_cycle064.cal")
mtd.deleteWorkspace("aligned")
StripPeaks("focussed","stripped")

# Plot a spectrum from each remaining workspace
g1 = plotInstrumentSpectrum("stripped",3)
g2 = plotInstrumentSpectrum("focussed",3)

# Insert the first curve from the active graph in g2 into active graph in g1
g1.insertCurve(g2,0)
g1.activeLayer().setScale(Layer.Bottom,0,2.3)
g2.close()

# Plot a spectrum from each remaining workspace
g3 = plotInstrumentSpectrum("stripped",2)
g4 = plotInstrumentSpectrum("focussed",2)

# Insert the first curve from the active graph in g2 into active graph in g1
g3.insertCurve(g4,0)
g3.activeLayer().setScale(Layer.Bottom,0,2.3)
g4.close()

print "Done!"
