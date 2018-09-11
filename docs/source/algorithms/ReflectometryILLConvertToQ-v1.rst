.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm is the last step in ILL's reflectometry reduction workflow. Its main purpose it convert a reflectivity workspace from wavelength to momentum transfer. This is achieved by :ref:`ReflectometryMomentumTransfer <algm-ReflectometryMomentumTransfer>` which also computes the :math:`Q_{z}` resolution. Further, histogrammed *InputWorkspace* is converted to point data by :ref:`ConvertToPointData <algm-ConvertToPointData>` and, optionally, the points are grouped according to the :math:`Q_{z}` resolution.

The diagram below shows the workflow of this algorithm:

.. diagram:: ReflectometryILLConvertToQ-v1_wkflw.dot

Usage
-----

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
       OutputBeamPositionWorkspace='direct_beam_pos',  # For reflected angle calibration.
       **settings
   )
   directFgd = ReflectometryILLSumForeground(direct.OutputWorkspace)

   # Reflected beam
   reflected = ReflectometryILLPreprocess(
       Run='ILL/D17/317370.nxs',
       DirectBeamPositionWorkspace='direct_beam_pos',
       **settings
   )

   reflectivityLambda = ReflectometryILLSumForeground(
       InputWorkspace=reflected,
       DirectForegroundWorkspace=directFgd,
       WavelengthRange=[2, 15],
   )
   reflectivityQ = ReflectometryILLConvertToQ(
       InputWorkspace=reflectivityLambda,
       ReflectedBeamWorkspace=reflected,            # Needed for Q resolution
       DirectBeamWorkspace=direct.OutputWorkspace,  # Needed for Q resolution
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
   Number of points in reflectivityQ: 189
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
       OutputBeamPositionWorkspace='direct_beam_pos',  # For reflected angle calibration.
       **settings
   )
   directFgd = ReflectometryILLSumForeground(direct.OutputWorkspace)

   # Reflected beam. Flippers set to '++'
   reflected11 = ReflectometryILLPreprocess(
       Run='ILL/D17/317370.nxs',
       DirectBeamPositionWorkspace='direct_beam_pos',
       **settings
   )

   reflectivity11 = ReflectometryILLSumForeground(
       InputWorkspace=reflected11,
       DirectForegroundWorkspace=directFgd,
       WavelengthRange=[2, 15]
   )
   # Reload the reflected be. We will fake the '--' flipper settings
   reflected00 = ReflectometryILLPreprocess(
       Run='ILL/D17/317370.nxs',
       DirectBeamPositionWorkspace='direct_beam_pos',
       **settings
   )

   reflectivity00 = ReflectometryILLSumForeground(
       InputWorkspace=reflected00,
       DirectForegroundWorkspace=directFgd,
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
       ReflectedBeamWorkspace=reflected00,          # Needed for Q resolution
       DirectBeamWorkspace=direct.OutputWorkspace,  # Needed for Q resolution
       Polarized=True,                              # Explicitly state it's polarized
       GroupingQFraction=0.4
   )
   R11 = ReflectometryILLConvertToQ(
       InputWorkspace=polcorr11,
       ReflectedBeamWorkspace=reflected11,          # Needed for Q resolution
       DirectBeamWorkspace=direct.OutputWorkspace,  # Needed for Q resolution
       Polarized=True,                              # Explicitly state it's polarized
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
   Number of points in R00: 259
   X unit in R11: MomentumTransfer
   Number of points in R11: 259
   Size of Q resolution data: 259

.. categories::

.. sourcelink::
