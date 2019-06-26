.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm is the last step in ILL's reflectometry reduction workflow. Its main purpose it convert a reflectivity workspace from wavelength to momentum transfer. This is achieved by :ref:`ReflectometryMomentumTransfer <algm-ReflectometryMomentumTransfer>` which also computes the :math:`Q_{z}` resolution. Further, histogrammed *InputWorkspace* is converted to point data by :ref:`ConvertToPointData <algm-ConvertToPointData>` and, optionally, the points are grouped according to the :math:`Q_{z}` resolution.

The diagram below shows the workflow of this algorithm:

.. diagram:: ReflectometryILLConvertToQ-v1_wkflw.dot

The algorithm expects to find a ``foreground.summation_type`` entry in the *InputWorkspace*'s sample logs containing either ``SumInLambda`` or ``SumInQ``. This entry is automatically added to the workspace by :ref:`ReflectometryILLSumForeground <algm-ReflectometryILLSumForeground>`.

Usage
-----

.. include:: ../usagedata-note.txt

**Example - nonpolarized reduction**

.. testcode:: NonpolarizedEx

   # Use same foreground and background settings for direct and reflected
   # beams.
   # Python dictionaries can be passed to algorithms as 'keyword arguments'.
   settings = {
       'ForegroundHalfWidth':[5],
       'LowAngleBkgOffset': 10,
       'LowAngleBkgWidth': 20,
       'HighAngleBkgOffset': 10,
       'HighAngleBkgWidth': 50,
   }
   
   # Direct beam
   direct = ReflectometryILLPreprocess(
       Run='ILL/D17/317369.nxs',
       **settings
   )
   directFgd = ReflectometryILLSumForeground(
       Inputworkspace=direct,
       WavelengthRange=[2, 15])
   
   # Reflected beam
   reflected = ReflectometryILLPreprocess(
       Run='ILL/D17/317370.nxs',
       DirectLineWorkspace=direct,
       **settings
   )
   
   reflectivityLambda = ReflectometryILLSumForeground(
       InputWorkspace=reflected,
       DirectForegroundWorkspace=directFgd,
       DirectLineWorkspace=direct,
       WavelengthRange=[2, 15],
   )
   reflectivityQ = ReflectometryILLConvertToQ(
       InputWorkspace=reflectivityLambda,
       # The next line is not needed if SumInQ was used in foreground summation
       DirectForegroundWorkspace=directFgd,         
       GroupingQFraction=0.4
   )
   
   # The data is now in Q
   print('Reflectivity X unit: ' + reflectivityQ.getAxis(0).getUnit().unitID())
   print('Is reflectivityLambda histogram? {}'.format(reflectivityLambda.isHistogramData()))
   print('Is reflectivityQ histogram? {}'.format(reflectivityQ.isHistogramData()))
   print('Number of bins in reflectivityLambda: {}'.format(len(reflectivityLambda.readX(0))))
   # There is a lot less points due to grouping
   print('Number of points in reflectivityQ: {}'.format(len(reflectivityQ.readX(0))))
   # The Q resolution is saved in the Dx field
   print('Has reflectivityQ Dx? {}'.format(reflectivityQ.hasDx(0)))

Output:

.. testoutput:: NonpolarizedEx

   Reflectivity X unit: MomentumTransfer
   Is reflectivityLambda histogram? True
   Is reflectivityQ histogram? False
   Number of bins in reflectivityLambda: 416
   Number of points in reflectivityQ: 190
   Has reflectivityQ Dx? True

**Example - polarized reduction**

.. testcode:: PolarizedEx

   # Use same foreground and background settings for direct and reflected
   # beams.
   # Python dictionaries can be passed to algorithms as 'keyword arguments'.
   settings = {
       'ForegroundHalfWidth':[5],
       'LowAngleBkgOffset': 10,
       'LowAngleBkgWidth': 20,
       'HighAngleBkgOffset': 10,
       'HighAngleBkgWidth': 50,
   }

   # Direct beam
   direct = ReflectometryILLPreprocess(
       Run='ILL/D17/317369.nxs',
       **settings
   )
   # For reflected angle calibration:
   directFgd = ReflectometryILLSumForeground(
       InputWorkspace=direct,
       WavelengthRange=[2, 15]
   )
   ReflectometryILLPolarizationCor(
       InputWorkspaces='directFgd',
       OutputWorkspace='pol_corrected_direct',  # Name of the group workspace
       EfficiencyFile='ILL/D17/PolarizationFactors.txt'
   )

   # Reflected beam. Flippers set to '++'
   reflected11 = ReflectometryILLPreprocess(
       Run='ILL/D17/317370.nxs',
       DirectLineWorkspace=direct,
       **settings
   )

   reflectivity11 = ReflectometryILLSumForeground(
       InputWorkspace=reflected11,
       DirectForegroundWorkspace='pol_corrected_direct_++',
       DirectLineWorkspace=direct,
       WavelengthRange=[2, 15]
   )
   # Reload the reflected beam. We will fake the '--' flipper settings
   reflected00 = ReflectometryILLPreprocess(
       Run='ILL/D17/317370.nxs',
       DirectLineWorkspace=direct,
       **settings
   )

   reflectivity00 = ReflectometryILLSumForeground(
       InputWorkspace=reflected00,
       DirectForegroundWorkspace='pol_corrected_direct_++',
       DirectLineWorkspace=direct,
       WavelengthRange=[2, 15]
   )
   # Overwrite sample logs
   replace = True
   logs = reflectivity00.mutableRun()
   logs.addProperty('Flipper1.state', '-', replace)
   logs.addProperty('Flipper1.stateint', 0, replace)
   logs.addProperty('Flipper2.state', '-', replace)
   logs.addProperty('Flipper2.stateint', 0, replace)

   # Polarization efficiency correction
   # The algorithm will think that the analyzer was off.
   ReflectometryILLPolarizationCor(
       InputWorkspaces='reflectivity00, reflectivity11',
       OutputWorkspace='pol_corrected',  # Name of the group workspace
       EfficiencyFile='ILL/D17/PolarizationFactors.txt'
   )
   # The polarization corrected workspaces get automatically generated names
   polcorr00 = mtd['pol_corrected_--']
   polcorr11 = mtd['pol_corrected_++']

   R00 = ReflectometryILLConvertToQ(
       InputWorkspace=polcorr00,
       # The next line is not needed if SumInQ was used in foreground summation
       DirectForegroundWorkspace='pol_corrected_direct_++',
       GroupingQFraction=0.4
   )
   R11 = ReflectometryILLConvertToQ(
       InputWorkspace=polcorr11,
       # The next line is not needed if SumInQ was used in foreground summation
       DirectForegroundWorkspace='pol_corrected_direct_++',
       GroupingQFraction=0.4
   )

   print('X unit in R00: ' + R00.getAxis(0).getUnit().unitID())
   print('Number of points in R00: {}'.format(len(R00.readX(0))))
   print('X unit in R11: ' + R11.getAxis(0).getUnit().unitID())
   print('Number of points in R11: {}'.format(len(R11.readX(0))))
   print('Size of Q resolution data: {}'.format(len(R11.readDx(0))))

Output:

.. testoutput:: PolarizedEx

   X unit in R00: MomentumTransfer
   Number of points in R00: 190
   X unit in R11: MomentumTransfer
   Number of points in R11: 190
   Size of Q resolution data: 190

.. categories::

.. sourcelink::
