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


Utilises :ref:`algm-EnggFocus` which performs a TOF to dSpacing
conversion using calibrated pixel positions, focuses the values in
dSpacing and then converts them back to TOF. :ref:`algm-EnggFocus`
also perform corrections with Vanadium data.

Then this algorithm calls :ref:`algm-EnggFitPeaks` (as a child
algorithm) which through a sequence of peak fits determines a linear
relationship between dSpacing and measured TOF values in terms of
DIFA, DIFC and TZERO values and provides the these parameters to the
Calibrate algorithm.

This algorithm provides an indirect calibration of the sample
position, that is, a calibration returned in terms of DIFA, DIFC and
ZERO rather than an actual new sample position (hence the reason for
'indirect').

The parameters DIFA, DIFC and ZERO are returned and can be retrieved
as output properties as well. If a name is given in
OutputParametersTableName the algorithm also produces a table
workspace with that name, containing the two output parameters.
Presently the DIFA parameter is always set to zero (see
:ref:`algm-EnggFitDIFCFromPeaks`).

The script EnggUtils included with Mantid can produce a GSAS
parameters file for the ENGIN-X instrument, given the DIFC and ZERO
parameters for the instrument banks as produced by EnggCalibrate. This
can be done with the write_ENGINX_GSAS_iparam_file() function as shown
in the usage example below.

See the algorithm :ref:`algm-EnggFocus` for details on the Vanadium
corrections.

.. categories::

.. sourcelink::

Usage
-----

.. include:: ../usagedata-note.txt

**Example - Calculate Difc and Zero for EnginX:**

.. testcode:: ExampleCalib

   out_tbl_name = 'out_params'
   ws_name = 'test_engg_data'
   Load('ENGINX00213855.nxs', OutputWorkspace=ws_name)

   # Using precalculated Vanadium corrections. To calculate from scrach see EnggVanadiumCorrections
   van_integ_ws = Load('ENGINX_precalculated_vanadium_run000236516_integration.nxs')
   van_curves_ws = Load('ENGINX_precalculated_vanadium_run000236516_bank_curves.nxs')

   difa1, difc1, tzero1, peaks1 = EnggCalibrate(InputWorkspace=ws_name,
                                                VanIntegrationWorkspace=van_integ_ws,
                                                VanCurvesWorkspace=van_curves_ws,
                                                ExpectedPeaks=[1.28, 2.1], Bank='1',
                                                OutputParametersTableName=out_tbl_name)

   difa1, difc2, tzero2, peaks2 = EnggCalibrate(InputWorkspace=ws_name,
                                                VanIntegrationWorkspace=van_integ_ws,
                                                VanCurvesWorkspace=van_curves_ws,
                                                ExpectedPeaks=[1.28, 2.1], Bank='2')

   # You can produce an instrument parameters (iparam) file for GSAS.
   # Note that this is very specific to ENGIN-X
   GSAS_iparm_fname = 'ENGIN_X_banks.prm'
   import EnggUtils
   EnggUtils.write_ENGINX_GSAS_iparam_file(GSAS_iparm_fname, bank_names=['North', 'South'],
                                           difc=[difc1, difc2], tzero=[tzero1, tzero2])

   import math
   print("DIFA1: {0}".format(difa1))
   delta = 2
   approx_difc1 = 18267
   difc1_ok = abs(difc1 - approx_difc1) <= delta
   print("DIFC1 is approximately (+/- {0}) {1}: {2}".format(delta, approx_difc1, difc1_ok))
   approx_tzero1 = 277
   tzero1_ok = abs(tzero1 - approx_tzero1) <= delta
   print("TZERO1 is approximately (+/- {0}) {1}: {2}".format(delta, approx_tzero1, tzero1_ok))
   tbl = mtd[out_tbl_name]

   tbl_values_ok = (abs(tbl.cell(0,1) - approx_difc1) <= delta) and (abs(tbl.cell(0,2) - approx_tzero1) <= delta)
   print("The output table has {0} row(s) and its values are as expected: {1}".format(tbl.rowCount(),
                                                                                      tbl_values_ok))

   import os
   print("Output GSAS iparam file was written? {0}".format(os.path.exists(GSAS_iparm_fname)))
   print("Number of lines of the GSAS iparam file: {0}".format(sum(1 for line in open(GSAS_iparm_fname))))

.. testcleanup:: ExampleCalib

   DeleteWorkspace(out_tbl_name)
   DeleteWorkspace(ws_name)
   DeleteWorkspace(van_integ_ws)
   DeleteWorkspace(van_curves_ws)

   os.remove(GSAS_iparm_fname)

Output:

.. testoutput:: ExampleCalib

   DIFA1: 0.0
   DIFC1 is approximately (+/- 2) 18267: True
   TZERO1 is approximately (+/- 2) 277: True
   The output table has 1 row(s) and its values are as expected: True
   Output GSAS iparam file was written? True
   Number of lines of the GSAS iparam file: 36
