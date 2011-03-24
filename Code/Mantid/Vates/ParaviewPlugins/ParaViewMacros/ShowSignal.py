##-------------------------------------------------------------------------
## Author: Owen Arnold @ ISIS/Tessella
## Date: 24/03/2011
## Purpose: Show signal cell data as surface plot. Sets color range between 0 and 3 for signal value.
##          
##-------------------------------------------------------------------------

try: paraview.simple
except: from paraview.simple import *
paraview.simple._DisableFirstRenderCameraReset()

activeSource = GetActiveSource()
display = GetDisplayProperties(activeSource)
lookupTable = GetLookupTableForArray( "signal", 1, RGBPoints=[0.0, 0.23000000000000001, 0.29899999999999999, 0.754, 3.0, 0.70599999999999996, 0.016, 0.14999999999999999], VectorMode='Magnitude', NanColor=[0.25, 0.0, 0.0], ColorSpace='Diverging', ScalarRangeInitialized=1.0, LockScalarRange=1 )

a1_signal_PiecewiseFunction = CreatePiecewiseFunction()

display.Representation = 'Surface'
display.ColorArrayName = 'signal'
display.LookupTable = lookupTable
display.ColorAttributeType = 'CELL_DATA'

Render()
