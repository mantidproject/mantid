.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

.. warning::

   This algorithm is being developed for a specific instrument. It
   might get changed or even removed without a notification, should
   instrument scientists decide to do so.


Fits a series of single peaks to a spectrum with an expected
diffraction pattern.  The pattern is specified by providing a list of
peak centres where Bragg peaks are expected. These expected values
are used as initial peak positions when fitting the peaks. The
expected dSpacing values can be given as an input string or in a file
with values separated by commas. These values can be in units of either
TOF or dSpacing, but **not** a mixture of the two. At least one of the
peaks in the list must lie inside the x limits of the input workspace.

The peaks are fitted one at a time. The peak centres given in the
property ExpectedPeaks are initially assumed to be in units of TOF.
If none lie inside the x limits of the input workspace, EnggFitPeaks
tries converting them to dSpacing, and if they still don't fit the data
then the algorithm fails.

After the conversion of units, the algorithm tries to fit (in
time-of-flight) a peak in the neighborhood of every expected peak
using a peak shape or function. The conversion is done following GSAS equations
(see :ref:`Unit Factory <Unit Factory>` for more details) if the workspace is focused (single
spectrum) and has a log entry named "difc", where the GSAS DIFC
parameter is expected. Otherwise the conversion of units is done as in
the Mantid :ref:`ConvertUnits <algm-ConvertUnits>`.


Otherwise the conversion of units is done as in
the Mantid :ref:`ConvertUnits <algm-ConvertUnits>`.

.. seealso:: :ref:`EnggFitTOFFromPeaks <algm-EnggFitTOFFromPeaks>`.

This algorithm currently fits (single) peaks of type
:ref:`Back2BackExponential <func-BackToBackExponential>`. Other
alternatives might be added as optional in the future (for example the
simpler :ref:`Gaussian <func-Gaussian>` or the more complex
*Bk2BkExpConvPV* or :ref:`IkedaCarpenterPV <func-IkedaCarpenterPV>`). To
produce an initial guess for the peak
function parameters this algorithm uses the :ref:`FindPeaks <algm-FindPeaks>` algorithm
starting from the expected peaks list given in the inputs.

The algorithm produces an output table workspace with information
about the peaks fitted. The table has one row per peak and several
columns for the different fitted parameters (and the errors of these
parameters). If a name is given in the input OutFittedPeaksTable, the
table will be available in the "analysis data service" (workspaces
window) with that name.

Usage
-----

**Example - Fitting two peaks:**

.. testcode:: ExTwoPeaks

   # Two BackB2Back exponential peaks
   peak1 = "name=BackToBackExponential,I=6000,A=0.05,B=0.025,X0=15000,S=100"
   peak2 = "name=BackToBackExponential,I=5000,A=0.05,B=0.025,X0=35000,S=100"
   bg = "name=FlatBackground,A0=20"

   # Create workpsace with the above peaks and a single detector pixel
   ws = CreateSampleWorkspace(Function="User Defined",
                              UserDefinedFunction=";".join([peak1, peak2, bg]),
                              NumBanks=1,
                              BankPixelWidth=1,
                              XMin=6000,
                              XMax=45000,
                              BinWidth=10)

   # Update instrument geometry to something that would allow converting to some sane dSpacing values
   EditInstrumentGeometry(Workspace = ws, L2 = [1.5], Polar = [90], PrimaryFlightPath = 50)

   # Run the algorithm. Defaults are shown below. Files entered must be in .csv format and if both ExpectedPeaks and ExpectedPeaksFromFile are entered, the latter will be used.

   peaks_tbl = EnggFitPeaks(ws, 0, [0.8, 1.9])


   # Print the results
   print("Number of peaks fitted: {0}".format(peaks_tbl.rowCount()))

Output:

.. testcleanup:: ExTwoPeaks

   DeleteWorkspace(peaks_tbl)

.. testoutput:: ExTwoPeaks

   Number of peaks fitted: 2

.. categories::

.. sourcelink::
