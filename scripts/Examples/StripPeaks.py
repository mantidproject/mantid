# MantidPlot example script
#
# Using StripPeaks + dialog boxes
#
import mantid.simpleapi as mantid

mantid.LoadRawDialog(OutputWorkspace="GEM40979")
alg = mantid.AlignDetectorsDialog("GEM40979", OutputWorkspace="aligned")
mantid.DeleteWorkspace("GEM40979")
calfile = alg.getPropertyValue("CalibrationFile")
mantid.DiffractionFocussing("aligned", calfile, OutputWorkspace="focussed")
mantid.DeleteWorkspace("aligned")
mantid.StripPeaks("focussed", OutputWorkspace="stripped")

# Plot a spectrum from each remaining workspace
g1 = mantid.plotSpectrum(["focussed", "stripped"], 5)

# Rescale the x-axis to show an interesting region
g1.activeLayer().setScale(Layer.Bottom, 0, 2.3)
print("Done!")
