# Load some raw files
LoadRaw("../../../Test/Data/HET15869.RAW","HET15869")
LoadRaw("../../../Test/Data/GEM38370.raw","GEM38370")

# Plot a spectrum from both files (this automatically imports the MantidMatrix but 
# it is minimised by default
g1 = plotSpectrum("HET15869",5)
g2 = plotSpectrum("GEM38370",5)

g3 = plotTimeBin("HET15869",10)
g4 = plotTimeBin("GEM38370",10)

# Insert the first curve from the active graph in g2 into active graph in g1
g1.insertCurve(g2,0)

g4.insertCurve(g3,0)

# Rescale the x-axis to an interesting point
# The axes are labeled Layer.Top, Layer.Bottom, Layer.Left, Layer.Right
g1.activeLayer().setScale(Layer.Bottom,5000,7500)

g4.activeLayer().setScale(Layer.Left,0,10)

# Hide graph 2 and 3
g2.hide()
g3.hide()
