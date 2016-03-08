.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

.. warning::

   This algorithm is being developed for a specific instrument. It
   might get changed or even removed without a notification, should
   instrument scientists decide to do so.


Fits a series of single peak to a spectrum with an expected
diffraction pattern.  The pattern is specified by providing a list of
dSpacing values where Bragg peaks are expected. The algorithm then
fits peaks in those areas using a peak fitting function. The dSpacing
values for ExpectedPeaks are then converted to time-of-flight (TOF)
(as in the Mantid :ref:`ConvertUnits <algm-ConvertUnits>`
algorithm). Expected dSpacing values can be given as an input string
or in a file with values separated by commas.

These values are used as start peak position in fit. It is these
adjusted peak TOF value positions that are fitted against
ExpectedPeaks dSpacing values according to:


.. math:: TOF = DIFC*d + TZERO


TZERO and DIFC can then be used within the GSAS program.  The
parameters DIFC and ZERO are returned and can be retrieved as output
properties as well. The DIFA coefficient (quadratic term on d) is not
considered in this version of the algorithm.

This algorithm currently fits (single) peaks of type
:ref:`Back2BackExponential <func-Back2BackExponential>`. Other
alternatives might be added as optional in the future (for example the
simpler :ref:`Gaussian <func-Gaussian>` or the more complex
:ref:`Bk2BkExpConvPV <func-Bk2BkExpConvPV` or :ref:`IkedaCarpenterPV
<func-IkedaCarpenterPV>`). To produce an initial guess for the peak
function parameters this algorithm uses the :ref:`FindPeaks
<algm-FindPeaks>` algorithm.

If a name is given in OutParametersTable this algorithm also produces
a table workspace with that name, containing the two fitted (DIFC,
ZERO) parameters. The algorithm produces an output table workspace
with information about the peaks fitted. The table has one row per
peak and several columns for the different fitted parameters (and the
errors of these parameters). If a name is given in the input
OutFittedPeaksTable, the table will be available in the "analysis data
service" (workspaces window) with that name.

Usage
-----

**Example - Fitting two peaks:**

.. testcode:: ExTwoPeaks

   # Two BackB2Back exponential peaks
   peak1 = "name=BackToBackExponential,I=6000,A=1,B=0.5,X0=15000,S=250"
   peak2 = "name=BackToBackExponential,I=5000,A=1,B=0.7,X0=35000,S=300"

   # Create workpsace with the above peaks and a single detector pixel
   ws = CreateSampleWorkspace(Function="User Defined",
                              UserDefinedFunction=peak1 + ";" + peak2,
                              NumBanks=1,
                              BankPixelWidth=1,
                              XMin=6000,
                              XMax=45000,
                              BinWidth=10)

   # Update instrument geometry to something that would allow converting to some sane dSpacing values
   EditInstrumentGeometry(Workspace = ws, L2 = [1.5], Polar = [90], PrimaryFlightPath = 50)

   # Run the algorithm. Defaults are shown below. Files entered must be in .csv format and if both ExpectedPeaks and ExpectedPeaksFromFile are entered, the latter will be used.

   out_tbl_name = 'peaks'
   difc, zero, peaks_tbl = EnggFitPeaks(ws, 0, [0.8, 1.9], OutParametersTable=out_tbl_name)


   # Print the results
   print "Difc: %.1f" % difc
   print "Zero: %.1f" % zero
   tbl = mtd[out_tbl_name]
   print "The output table has %d row(s)" % tbl.rowCount()
   print "Parameters from the table, Difc: %.1f, Zero: %.1f" % (tbl.cell(0,0), tbl.cell(0,1))
   print "Number of peaks fitted: {0}".format(peaks_tbl.rowCount())
   print "First peak expected (dSpacing): {0}".format(peaks_tbl.column('dSpacing')[0])
   print "First fitted peak center (ToF): {0:.1f}".format(peaks_tbl.column('X0')[0])
   print "Second peak expected (dSpacing): {0}".format(peaks_tbl.column('dSpacing')[1])
   print "Second fitted peak center (ToF): {0:.1f}".format(peaks_tbl.column('X0')[1])

Output:

.. testcleanup:: ExTwoPeaks

   DeleteWorkspace(out_tbl_name)

.. testoutput:: ExTwoPeaks

   Difc: 18181.8
   Zero: 460.5
   The output table has 1 row(s)
   Parameters from the table, Difc: 18181.8, Zero: 460.5
   Number of peaks fitted: 2
   First peak expected (dSpacing): 0.8
   First fitted peak center (ToF): 15006.0
   Second peak expected (dSpacing): 1.9
   Second fitted peak center (ToF): 35006.0

.. categories::

.. sourcelink::
