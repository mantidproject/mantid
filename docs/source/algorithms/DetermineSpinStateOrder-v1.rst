
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Takes a Polarised SANS transmission run and attempts to determine the spin state for each run period.

The average current through the RF flipper for every run is extracted from the appropriate spin flipper log in the instrument. The beam polarisation is determined by
comparing each period transmission to the average transmission (greater than the average suggests the same state as the flipper).

If the data is from LARMOR or ZOOM, properties **SpinFlipperLogName** and **SpinFlipperAverageCurrent** can be inferred by the algorithm.

+-----------+----------------------+---------------------------------+
|Instrument | **SpinFlipperLogName** | **SpinFlipperAverageCurrent** |
+====================================+===============================+
|LARMOR     | ``FlipperCurrent``     | ``4.0``                       |
+------------------------------------+-------------------------------+
|ZOOM       | ``Spin_flipper``       | ``0.0``                       |
+-----------+------------------------+-------------------------------+

Otherwise, they can be supplied explicitly by the user.

Usage
-----

**Example - DetermineSpinStateOrder**

.. testcode:: DetermineSpinStateOrderExample

   from mantid.simpleapi import *
   from mantid.api import WorkspaceGroup

   wsGroup = WorkspaceGroup()
   mtd.add("group", wsGroup)
   y_values = [10, 80, 20, 90]
   flipper_current_values = [3, 3.5, 5.5, 5.8]

   for i in range(4):
       ws_name = f"ws_{i}"
       CreateSampleWorkspace("Histogram", "Flat background", XUnit="Wavelength", XMin=0, XMax=10, BinWidth=1, NumEvents=10, InstrumentName="LARMOR", OutputWorkspace=ws_name)
       CropWorkspace(ws_name, StartWorkspaceIndex=1, EndWorkspaceIndex=1, OutputWorkspace=ws_name)
       mtd[ws_name] *= y_values[i]
       AddTimeSeriesLog(ws_name, "FlipperCurrent", "2010-01-01T00:00:00", flipper_current_values[i])
       wsGroup.addWorkspace(mtd[ws_name])

   ConvertToHistogram(wsGroup, OutputWorkspace=wsGroup)
   result = DetermineSpinStateOrder(wsGroup)
   print(f"Spin state order from wsGroup is {result}")

Output:

.. testoutput:: DetermineSpinStateOrderExample

   Spin state order from wsGroup is 01,00,10,11

.. categories::

.. sourcelink::
