# -----------------------------------------------------
#  Runs PlotAssymetryByLogValue algorithm
#  and plots the output in MantidPlot using a Python
#  dictionary
#------------------------------------------------------

# Execute the algorithm
alg = PlotAsymmetryByLogValueDialog()

# Get the output workspace
ws = alg.getPropertyValue("OutputWorkspace")

spectra_plot = { 0 : 'Difference' }
if int(alg.getPropertyValue("Green")) < 1000:
    spectraplot[1] = 'Red'
    spectraplot[2] = 'Green'
    spectraplot[3] = 'Sum'

gs = plotSpectrum(ws, spectra_plot.keys())
for key, value in spectra_plot.iteritems():
    gs.activeLayer().setCurveTitle(key, value)
