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
   ws_name = 'test'
   Load('ENGINX00213855.nxs', OutputWorkspace=ws_name)

   # Using precalculated Vanadium corrections. To calculate from scrach see EnggVanadiumCorrections
   van_integ_ws = Load('ENGINX_precalculated_vanadium_run000236516_integration.nxs')
   van_curves_ws = Load('ENGINX_precalculated_vanadium_run000236516_bank_curves.nxs')

   difa1, Difc1, Zero1, peaks1 = EnggCalibrate(InputWorkspace=ws_name,
                                               VanIntegrationWorkspace=van_integ_ws,
                                               VanCurvesWorkspace=van_curves_ws,
                                               ExpectedPeaks=[1.097, 2.1], Bank='1',
                                               OutputParametersTableName=out_tbl_name)

   Difa1, Difc2, Zero2, peaks2 = EnggCalibrate(InputWorkspace=ws_name,
                                               VanIntegrationWorkspace=van_integ_ws,
                                               VanCurvesWorkspace=van_curves_ws,
                                               ExpectedPeaks=[1.097, 2.1], Bank='2')

   # You can produce an instrument parameters (iparam) file for GSAS.
   # Note that this is very specific to ENGIN-X
   GSAS_iparm_fname = 'ENGIN_X_bank1'
   import EnggUtils
   EnggUtils.write_ENGINX_GSAS_iparam_file(GSAS_iparm_fname, [Difc1, Difc2], [Zero1, Zero2])

   print "DIFA1: %.2f" % (Difa1)
   print "DIFC1: %.2f" % (Difc1)
   print "TZERO1: %.2f" % (Zero1)
   tbl = mtd[out_tbl_name]
   print "The output table has %d row(s)" % tbl.rowCount()
   print "Parameters from the table, DIFC1: %.2f, ZERO1: %.2f" % (tbl.cell(0,1), tbl.cell(0,2))
   import os
   print "Output GSAS iparam file was written?", os.path.exists(GSAS_iparm_fname)
   print "Number of lines of the GSAS iparam file:", sum(1 for line in open(GSAS_iparm_fname))

.. testcleanup:: ExampleCalib

   DeleteWorkspace(out_tbl_name)
   DeleteWorkspace(ws_name)
   DeleteWorkspace(van_integ_ws)
   DeleteWorkspace(van_curves_ws)

   os.remove(GSAS_iparm_fname)

Output:

.. testoutput:: ExampleCalib

   DIFA1: 0.00
   DIFC1: 18400.19
   TZERO1: -2.06
   The output table has 1 row(s)
   Parameters from the table, DIFC1: 18400.19, ZERO1: -2.06
   Output GSAS iparam file was written? True
   Number of lines of the GSAS iparam file: 35
