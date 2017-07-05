# -----------------------------------------------------
#  Runs PlotAsymmetryByLogValue algorithm
#  and plots the output in MantidPlot using a Python
#  dictionary
# ------------------------------------------------------
import mantid.simpleapi as mantid
from six import iteritems

# Execute the algorithm
alg = mantid.PlotAsymmetryByLogValueDialog()

# Get the output workspace
ws = alg.getPropertyValue("OutputWorkspace")

spectra_plot = {0: 'Difference'}
if int(alg.getPropertyValue("Green")) < 1000:
    spectra_plot[1] = 'Red'
    spectra_plot[2] = 'Green'
    spectra_plot[3] = 'Sum'

gs = mantid.plotSpectrum(ws, spectra_plot.keys())
for key, value in iteritems(spectra_plot):
    gs.activeLayer().setCurveTitle(key, value)
