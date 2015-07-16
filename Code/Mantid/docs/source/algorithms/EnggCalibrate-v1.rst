.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

.. warning::

   This algorithm is being developed for a specific instrument. It might get changed or even 
   removed without a notification, should instrument scientists decide to do so.


Utilises :ref:`algm-EnggFocus` which performs a TOF to dSpacing
conversion using calibrated pixel positions, focuses the values in
dSpacing and then converts them back to TOF. :ref:`algm-EnginXFocus`
also perform corrections with Vanadium data.

Then this algorithm calls :ref:`algm-EnggFitPeaks` (as a child
algorithm) which through a sequence of peak fits determines a linear
relationship between dSpacing and measured TOF values in terms of DIFC
and ZERO values and provides the these parameters to the Calibrate
algorithm.

This algorithm provides an indirect calibration of the sample
position, that is, a calibration returned in terms of Difc and Zero
rather than an actual new sample position (hence the reason for
'indirect').

The parameters DIFC and ZERO are returned and can be retrieved as
output properties as well. If a name is given in
OutputParametersTableName the algorithm also produces a table
workspace with that name, containing the two output parameters.

See the algorithm :ref:`algm-EnginXFocus` for details on the Vanadium
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
   van_ws_name = 'test_vanadium'
   Load('ENGINX00213855.nxs', OutputWorkspace=ws_name)
   Load('ENGINX00193749.nxs', OutputWorkspace=van_ws_name)
   Difc, Zero = EnggCalibrate(InputWorkspace=ws_name,
                              VanadiumWorkspace= van_ws_name,
                              ExpectedPeaks=[1.097, 2.1], Bank='1',
                              OutputParametersTableName=out_tbl_name)

   print "Difc: %.2f" % (Difc)
   print "Zero: %.2f" % (Zero)
   tbl = mtd[out_tbl_name]
   print "The output table has %d row(s)" % tbl.rowCount()
   print "Parameters from the table, Difc: %.2f, Zero: %.2f" % (tbl.cell(0,0), tbl.cell(0,1))

.. testcleanup:: ExampleCalib

   DeleteWorkspace(out_tbl_name)
   DeleteWorkspace(ws_name)
   DeleteWorkspace(van_ws_name)

Output:

.. testoutput:: ExampleCalib

   Difc: 18404.35
   Zero: -8.77
   The output table has 1 row(s)
   Parameters from the table, Difc: 18404.35, Zero: -8.77
