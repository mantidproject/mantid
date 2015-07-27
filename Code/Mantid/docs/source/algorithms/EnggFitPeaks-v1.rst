.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

.. warning::

   This algorithm is being developed for a specific instrument. It might get changed or even
   removed without a notification, should instrument scientists decide to do so.


The pattern is specified by providing a list of dSpacing values where
Bragg peaks are expected. The algorithm then fits peaks in those areas
using a peak fitting function. The dSpacing values for ExpectedPeaks
are then converted to time-of-flight (TOF) (as in the Mantid
ConvertUnits algorithm).

These values are used as start peak position in fit. It is these
adjusted peak TOF value positions that are fitted against
ExpectedPeaks dSpacing values according to:


.. math:: TOF = DifC*d + Zero


ZERO and Difc can then be used within the GSAS program.  The
parameters DIFC and ZERO are returned and can be retrieved as output
properties as well.

If a name is given in OutParametersTable this algorithm also
produces a table workspace with that name, containing the two fitted
(DIFC, ZERO) parameters. Also, if a name is given in
OutFittedPeaksTable, the algorithm produces a table workspace with
information about the peaks fitted. The table has one row per peak and
several columns for the different fitted parameters (and the errors of
these parameters).

Usage
-----

**Example - Fitting two peaks:**

.. testcode:: ExTwoPeaks

   # Two B2B peaks
   peak1 = "name=BackToBackExponential,I=4000,A=1,B=0.5,X0=12000,S=350"
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
   # difc, zero = EnggXFitPeaks(InputWorkspace = No default, WorkspaceIndex = None, ExpectedPeaks=[0.6, 1.9], ExpectedPeaksFromFile=None)

   out_tbl_name = 'out_params'
   difc, zero = EnggFitPeaks(ws, 0, [0.65, 1.9], OutParametersTable=out_tbl_name)


   # Print the results
   print "Difc: %.1f" % difc
   print "Zero: %.1f" % zero
   tbl = mtd[out_tbl_name]
   print "The output table has %d row(s)" % tbl.rowCount()
   print "Parameters from the table, Difc: %.1f, Zero: %.1f" % (tbl.cell(0,0), tbl.cell(0,1))

Output:

.. testcleanup:: ExTwoPeaks

   DeleteWorkspace(out_tbl_name)

.. testoutput:: ExTwoPeaks

   Difc: 18400.0
   Zero: 46.0
   The output table has 1 row(s)
   Parameters from the table, Difc: 18400.0, Zero: 46.0

.. categories::

.. sourcelink::
