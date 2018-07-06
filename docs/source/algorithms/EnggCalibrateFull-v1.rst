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


Allows to calibrate or correct for variations in detector position
parameters. It does this by fitting the peaks for each of the selected
bank's detector indices, (using :ref:`algm-EnggFitPeaks` as a child
algorithm) and using the resulting DIFC (GSAS calibration parameter)
values to calibrate the detector positions.

This algorithm produces a table with calibration information,
including calibrated or corrected positions and parameters. This
calibration information that can be inspected and can also be used in
other algorithms. The algorithm also applies the calibration on the
input workspace. After running this algorithm the resulting
calibration can be checked by visualizing the input workspace
instrument in the instrument viewer and/or inspecting the output table
values.

The output table has one row per detector where each row gives the
original position before calibration (as a V3D point, x-y-z values),
the new calibrated position (as V3D) and the calibrated spherical
co-ordinates (L2, :math:`2 \theta`, :math:`\phi`). It also gives the
variation in the L2 position, and the 'DIFA', 'DIFC' and 'TZERO'
calibration parameters as used in GSAS and other Mantid algorithms.

The result of the calibration (the output table given in
OutDetPosTable) is accepted by both :ref:`algm-EnggCalibrate` and
:ref:`algm-EnggFocus` which use the columns 'Detector ID' and
'Detector Position' of the table to correct the detector positions
before focussing. The OutDetPosTable output table can also be used
to apply the calibration calculated by this algorithm on any other
workspace by using the algorithm :ref:`algm-ApplyCalibration`.

In the output table the calibrated positions for every detector are
found by calculating the *L2* values from the *DIFC* values as
follows:

.. math:: L2 = \left(\frac{DIFC} { 252.816 * 2 * sin \left(\frac{2\theta} {2.0}\right)}\right) - 50

where the *DIFC* values are obtained from the fitting of expected
peaks. See the algorithm :ref:`algm-EnggFitPeaks` for details on how
*DIFC* and other calibration parameters are calculated.

This algorithm expects as input/output workspace the *long*
calibration run, which provides a decent pattern for every detector or
pixel. The spectra of the workspace are corrected using data from a
Vanadium run (passed in the VanadiumWorkspace property). These
corrections include two steps: detector sensitivity correction and
pixel-by-pixel correction on a per-bank basis. See also
:ref:`algm-EnggFocus` where the same correction is applied. Similarly
as in :ref:`algm-EnggFocus`, it is possible to retrieve the curves
fitted to the Vanadium run data for every bank by using the
OutVanadiumCurveFits property.

.. categories::

.. sourcelink::

Usage
-----

.. include:: ../usagedata-note.txt

**Example - Calculate corrected positions, using the DIFC parameter, for the EnginX instrument:**

.. testcode:: ExCalFull

   # Using a workspace with only one spectrum to keep this test small and fast.
   # Normally you would use a long Ceria run file or similar
   ws_name = 'ws_focussed'
   Load('ENGINX00213855focussed.nxs', OutputWorkspace=ws_name)

   # Using precalculated Vanadium corrections. To calculate from scrach see EnggVanadiumCorrections
   van_integ_ws = Load('ENGINX_precalculated_vanadium_run000236516_integration.nxs')
   van_curves_ws = Load('ENGINX_precalculated_vanadium_run000236516_bank_curves.nxs')

   pos_table, peaks_info = EnggCalibrateFull(Workspace=ws_name,
                                             VanIntegrationWorkspace=van_integ_ws,
                                             VanCurvesWorkspace=van_curves_ws,
                                             ExpectedPeaks=[1.097, 2.1], Bank='1')

   det_id = pos_table.column(0)[0]
   cal_pos =  pos_table.column(2)[0]
   print("Det ID: {}".format(det_id))
   print("Calibrated position: (%.3f,%.3f,%.3f)" % (cal_pos.getX(), cal_pos.getY(), cal_pos.getZ()))
   ws = mtd[ws_name]
   posInWSInst = ws.getInstrument().getDetector(det_id).getPos()
   print("Is the detector position calibrated now in the original workspace instrument? {}".format(cal_pos == posInWSInst))

.. testcleanup:: ExCalFull

   DeleteWorkspace(ws_name)
   DeleteWorkspace(van_integ_ws)
   DeleteWorkspace(van_curves_ws)

Output:

.. testoutput:: ExCalFull

   Det ID: 100001
   Calibrated position: (1.506,0.000,0.002)
   Is the detector position calibrated now in the original workspace instrument? True

**Example - Calculate corrected positions for EngingX, saving in a file:**

.. testcode:: ExCalFullWithOutputFile

   import os, csv

   ws_name = 'ws_focussed'
   pos_filename = 'detectors_pos.csv'
   # Note that this is a small file which is not very meaningful but simple enough for
   # this test to run fast. Please user your (proper) run file.
   Load('ENGINX00213855focussed.nxs', OutputWorkspace=ws_name)

   # Using precalculated Vanadium corrections. To calculate from scrach see EnggVanadiumCorrections
   van_integ_ws = Load('ENGINX_precalculated_vanadium_run000236516_integration.nxs')
   van_curves_ws = Load('ENGINX_precalculated_vanadium_run000236516_bank_curves.nxs')

   pos_table, peaks_info = EnggCalibrateFull(Workspace=ws_name,
                                             VanIntegrationWorkspace=van_integ_ws,
                                             VanCurvesWorkspace=van_curves_ws,
                                             ExpectedPeaks=[1.097, 2.1], Bank='1',
                                             OutDetPosFilename=pos_filename)

   det_id = pos_table.column(0)[0]
   pos =  pos_table.column(2)[0]
   print("Det ID: {}".format(det_id))
   print("Calibrated position: ({0:.3f},{1:.3f},{2:.3f})".format(pos.getX(),pos.getY(),pos.getZ()))
   print("Got details on the peaks fitted for {0:d} detector(s)".format(peaks_info.rowCount()))
   print("Was the file created? {}".format(os.path.exists(pos_filename)))
   with open(pos_filename) as csvf:
      reader = csv.reader(csvf, dialect='excel')
      next(reader)
      calibOK = True
      for i,row in enumerate(reader):
         cal_pos = pos_table.column(2)[i]
         calibOK = calibOK and (abs(float(row[4]) - cal_pos.getX()) < 1e-6) and\
                   (abs(float(row[5]) - cal_pos.getY()) < 1e-6) and\
                   (abs(float(row[6]) - cal_pos.getZ()) < 1e-6)
         if not calibOK: break
   print("Does the calibration file have the expected values? {}".format(calibOK))

.. testcleanup:: ExCalFullWithOutputFile

   DeleteWorkspace(ws_name)
   DeleteWorkspace(van_integ_ws)
   DeleteWorkspace(van_curves_ws)
   import os
   os.remove(pos_filename)

Output:

.. testoutput:: ExCalFullWithOutputFile

   Det ID: 100001
   Calibrated position: (1.506,0.000,0.002)
   Got details on the peaks fitted for 1 detector(s)
   Was the file created? True
   Does the calibration file have the expected values? True
