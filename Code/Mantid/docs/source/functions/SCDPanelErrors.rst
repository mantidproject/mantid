==============
SCDPanelErrors
==============


Description
-----------

-  This fit function is used for calibrating RectangularDetectors by
   adjusting L0, time offset, panel width,

panel height, panel center, panel orientation, and allow for the sample
being offset from the instrument center.

Attributes
~~~~~~~~~~

This fit function is used for calibrating RectangularDetectors by
adjusting L0, time offset, panel width, panel height, panel center,
panel orientation, and allow for the sample being offset from the
instrument center.

Attributes
~~~~~~~~~~

-  a -The lattice parameter a
-  b -The lattice parameter b
-  c -The lattice parameter c
-  alpha -The lattice parameter alpha in degrees
-  beta -The lattice parameter beta in degrees
-  gamma -The lattice parameter gamma in degrees
-  PeakWorkspaceName-The name of the PeaksWorkspace in the Analysis Data
   Service.

    This peak must be indexed by a UB matrix whose lattice parameters
    are CLOSE to the above
    lattice paramters

-  NGroups-The number of grouping of banks to be considered
-  BankNames-a list of banknames separated by "/" or a "!" if the next
   bank is in a different group.

    Bank names from the same group belong together("Requirement" for use
    with the Fit algorithm)

-  startX- -1 or starting position in the workspace( see below) to start
   calculating the outputs
-  endX- -1 or 1+ ending position in the workspace( see below) to start
   calculating the outputs
-  RotateCenters-Boolean. If false Rotations are only about the center
   of the banks. Otherwise rotations are ALSO

    around center of the instrument( For groups of banks, this will
    result in a rotation about the center of all pixels.)

-  SampleOffsets-Boolean. A sample being off from the center of the
   goniometer can result in larger errors.

Workspace
~~~~~~~~~

A Workspace2D with 1 spectra. The xvalues of the spectra are for each
peak, the peak index repeated 3 times. The y values are all zero and the
errors are all 1.0

This spectra may have to be copied 3 times because of requirements from
the fitting system.

Parameters
~~~~~~~~~~

-  l0- the initial Flight path in units from Peak.getL1
-  t0-Time offset in the same units returned with Peak.getTOF)
-  SampleX-Sample x offset in the same units returned with
   Peak.getDetPos().norm()
-  SampleY-Sample y offset in the same units returned with
   Peak.getDetPos().norm()
-  SampleZ-Sample z offset in the same units returned with
   Peak.getDetPos().norm()
-  f\*\_detWidthScale-panel Width for Group\* in the same units returned
   with Peak.getDetPos().norm()
-  f\*\_detHeightScale-panel Height for Group\* in the same units
   returned with Peak.getDetPos().norm()
-  f\*\_Xoffset-Panel Center x offsets for Group\* banks in the same
   units returned with Peak.getDetPos().norm()
-  f\*\_Yoffset-Panel Center y offsets for Group\* banks in the same
   units returned with Peak.getDetPos().norm()
-  f\*\_Zoffset-Panel Center z offsets for Group\* banks in the same
   units returned with Peak.getDetPos().norm()
-  f\*\_Xrot-Rotations(degrees) for Group\* banks around "Center" in x
   axis direction
-  f\*\_Yrot-Rotations(degrees) for Group\* banks around "Center" in y
   axis direction
-  f\*\_Zrot-Rotations(degrees) for Group\* banks around "Center" in z
   axis direction
-  SampleX -sample X offset in meters(Really Goniometer X offset)
-  SampleY- sample Y offset in meters
-  SampleZ- sample Z offset in meters

The order of rotations correspond to the order used in all of Mantid.

Output
~~~~~~

The argument out from function1D ,for each peak, gives the error in qx,
qy, and qz. The theoretical values for the qx, qy and qz are found as
follows:

-  Calculating the best fitting UB for the given indexing and parameter
   values
-  Find U
-  The theoretical UB is then U\*B\ :sub:`0` where B\ :sub:`0` is formed
   from the supplied lattice parameters
-  The theoretical qx,qy,and qz can be obtained by multiplying the hkl
   for the peak by this matrix(/2π)
