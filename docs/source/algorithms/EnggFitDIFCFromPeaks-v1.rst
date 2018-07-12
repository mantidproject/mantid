.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

.. warning::

   This algorithm is being developed for a specific instrument. It
   might undergo significant changes, should instrument scientists
   decide to do so.

Finds the calibration parameters DIFC and TZERO (as defined in GSAS)
from a list of peaks fitted to a diffraction pattern. The peaks can be
fitted to a Mantid workspace spectrum using the algorithm
:ref:`EnggFitPeaks <algm-EnggFitPeaks>` which produces a table with
parameter values for peaks in time-of-flight (TOF, see
:ref:`Unit Factory <Unit Factory>`). The table is the essential
input to this algorithm.

This algorithm fits the adjusted peak time-of-fligth value positions
that are fitted against expected dSpacing values (d) according to the
expression:

.. math:: TOF = DIFC*d + TZERO

The calibration parameters TZERO and DIFC can then be used within the
GSAS program or in other Mantid algorithms (see :ref:`EnggCalibrate
<algm-EnggCalibrate>` and :ref:`EnggCalibrateFull
<algm-EnggCalibrateFull>`).  These parameters are returned and can be
retrieved as output properties as well. The DIFA coefficient
(quadratic term on d) is not considered and is fixed to zero in this
version of the algorithm.

If a name is given in OutParametersTable this algorithm also produces
a table workspace with that name, containing the parameters fitted
(DIFA, DIFC, TZERO).

The parameters DIFA, DIFC, TZERO are also used in other Mantid
algorithms. For example see :ref:`AlignDetectors
<algm-AlignDetectors>` where these parameters are used to convert
units from time-of-flight to d-spacing.

Usage
-----

**Example - Fitting DIFC parameter with two peaks:**

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

   peaks_tbl = EnggFitPeaks(ws, 0, [0.8, 1.9])
   out_tbl_name = 'difc_from_peaks'
   difa, difc, tzero = EnggFitDIFCFromPeaks(FittedPeaks=peaks_tbl, OutParametersTable=out_tbl_name)

   # Print the results
   print("DIFA: %.1f" % difa)
   print("DIFC: %.0f" % round(difc,-1))
   print("TZERO: %.0f" %round(tzero,-1))
   tbl = mtd[out_tbl_name]
   print("The output table has %d row(s)" % tbl.rowCount())
   print("Parameters from the table, DIFA: %.1f, DIFC: %.0f, TZERO: %.0f" % (tbl.cell(0,0), round(tbl.cell(0,1),-1), round(tbl.cell(0,2),-1)))
   print("Number of peaks fitted: {0}".format(peaks_tbl.rowCount()))
   print("First peak expected (dSpacing): {0}".format(peaks_tbl.column('dSpacing')[0]))
   print("First fitted peak center (ToF): {0:.1f}".format(peaks_tbl.column('X0')[0]))
   print("Second peak expected (dSpacing): {0}".format(peaks_tbl.column('dSpacing')[1]))
   print("Second fitted peak center (ToF): {0:.0f}".format(round(peaks_tbl.column('X0')[1],-1)))

Output:

.. testcleanup:: ExTwoPeaks

   DeleteWorkspace(out_tbl_name)

.. testoutput:: ExTwoPeaks

   DIFA: 0.0
   DIFC: 18180
   TZERO: 460
   The output table has 1 row(s)
   Parameters from the table, DIFA: 0.0, DIFC: 18180, TZERO: 460
   Number of peaks fitted: 2
   First peak expected (dSpacing): 0.8
   First fitted peak center (ToF): 15006.0
   Second peak expected (dSpacing): 1.9
   Second fitted peak center (ToF): 35010

.. categories::

.. sourcelink::
