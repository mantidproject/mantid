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

This algorithm finds the calibration parameters DIFA, DIFC and TZERO (as defined in GSAS)
from a list of peaks fitted to a diffraction pattern. The peaks can be
fitted to a Mantid workspace spectrum using the algorithm
:ref:`EnggFitPeaks <algm-EnggFitPeaks>` which produces a table with
parameter values for peaks in time-of-flight (TOF, see
:ref:`Unit Factory <Unit Factory>`). The table is the essential
input to this algorithm.

This algorithm fits the adjusted peak time-of-flight value positions
that are fitted against expected dSpacing values (d) according to the
expression:

.. math:: TOF = DIFA*d^2 + DIFC*d + TZERO

The calibration parameters DIFA, TZERO and DIFC can then be used within the
GSAS program or in other Mantid algorithms (see :ref:`EnggCalibrate
<algm-EnggCalibrate>` and :ref:`EnggCalibrateFull
<algm-EnggCalibrateFull>`).  These parameters are returned and can be
retrieved as output properties as well.

If a name is given in OutParametersTable this algorithm also produces
a table workspace with that name, containing the parameters fitted
(DIFA, DIFC, TZERO).

The parameters DIFA, DIFC, TZERO are also used in other Mantid algorithms. See
:ref:`Unit Factory <Unit Factory>` for more details.

Usage
-----

**Example - Fitting DIFC parameter with two peaks:**

.. testcode:: ExTwoPeaks

    # Three Back2Back exponential peaks
    peak1 = "name=BackToBackExponential,I=6000,A=0.05,B=0.025,X0=15000,S=100"
    peak2 = "name=BackToBackExponential,I=6000,A=0.05,B=0.025,X0=27500,S=100"
    peak3 = "name=BackToBackExponential,I=5000,A=0.05,B=0.025,X0=35000,S=100"
    bg = "name=FlatBackground,A0=20"

    # Create workpsace with the above peaks and a single detector pixel
    ws = CreateSampleWorkspace(Function="User Defined",
                              UserDefinedFunction=";".join([peak1, peak2, peak3, bg]),
                              NumBanks=1,
                              BankPixelWidth=1,
                              XMin=6000,
                              XMax=45000,
                              BinWidth=10)

    # Update instrument geometry to something that would allow converting to some sane dSpacing values
    EditInstrumentGeometry(Workspace = ws, L2 = [1.5], Polar = [90], PrimaryFlightPath = 50)

    # Run the algorithm. Defaults are shown below. Files entered must be in .csv format and if both ExpectedPeaks and ExpectedPeaksFromFile are entered, the latter will be used.

    peaks_tbl = EnggFitPeaks(ws, 0, [0.8, 1.5, 1.9])
    out_tbl_name = 'difc_from_peaks'
    difa, difc, tzero = EnggFitTOFFromPeaks(FittedPeaks=peaks_tbl, OutParametersTable=out_tbl_name)

    # Print the results
    print("DIFA: %.1f" % difa)
    print("DIFC: %.0f" % round(difc,-1))
    print("TZERO: %.0f" % round(tzero,-1))
    tbl = mtd[out_tbl_name]
    print("The output table has %d row(s)" % tbl.rowCount())
    print("Number of peaks fitted: {0}".format(peaks_tbl.rowCount()))

Output:

.. testcleanup:: ExTwoPeaks

   DeleteWorkspace(out_tbl_name)

.. testoutput:: ExTwoPeaks
   :options: +ELLIPSIS

   DIFA: ...
   DIFC: ...
   TZERO: ...
   The output table has 1 row(s)
   Number of peaks fitted: 3

.. categories::

.. sourcelink::
