# Load some raw files
LoadRaw("../../../Test/Data/HET15869.RAW","HET15869")
LoadRaw("../../../Test/Data/GEM38370.raw","GEM38370")

# Display matrices
# Note that this is can be skipped if you only wish to plot
# the spectrum
mm1 = newMantidMatrix("HET15869")
mm2 = newMantidMatrix("GEM38370")

# Plot a spectrum from both files
g1 = plotInstrumentSpectrum("HET15869",5)
g2 = plotInstrumentSpectrum("GEM38370",5)

# Insert the first curve from graph 2 into graph 1 window
g1.insertCurve(g2,0)

# Rescale the x-axis to an interesting point
# The axes are labeled Layer.Top, Layer.Bottom, Layer.Left, Layer.Right
# At the moment the graph 2 must be closed manually
g1.setScale(Layer.Bottom,5000,7500)

# Close graph 2
closeGraph(g2);

