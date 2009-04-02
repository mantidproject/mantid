######################################################
#
# Applies the current mask values from the GUI
# and shows them in the instrument window
#
# (Note: Due to a bug with plotting masked data on
# its own, the empty layer is added to some real data)
# 
#####################################################
import LOQFunctions

topLayer = 'CurrentMask'
LoadEmptyInstrument('<INSTRUMENTPATH>/LOQ_Definition.xml',topLayer)

rmin = <RADIUSMIN>/1000.0
LOQFunctions.MaskInsideCylinder(topLayer, rmin)

rmax = <RADIUSMAX>/1000.0
LOQFunctions.MaskOutsideCylinder(topLayer, rmax)

# Masking other spectra
# This assumes a list of comma separated values
maskstring = '<MASKLIST>'
detlist = LOQFunctions.ConvertToDetList(maskstring);
LOQFunctions.MaskByDetNumber(topLayer, detlist)

# Find the zeroed spectrum and mark them with a value of 500 and mark everything else
# with a value of 0
FindDeadDetectors(topLayer, topLayer, DeadValue='500')

# Visualise the result
insView = getInstrumentView(topLayer)
insView.showWindow()
