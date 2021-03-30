.. _func-VesuvioResolution:

.. index:: VesuvioResolution

.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Provides a simpler interface to the VesuvioResolution fit function for
calculating the resolution function for a given mass for VESUVIO data.

Usage
-----

**Example: Calculating resolution function**

.. testcode::

   ###### Simulates LoadVesuvio with spectrum number 136 ####################
   tof_ws = CreateSimulationWorkspace(Instrument='Vesuvio', BinParams=[50,0.5,562], UnitX='TOF')
   tof_ws = CropWorkspace(tof_ws, StartWorkspaceIndex=135, EndWorkspaceIndex=135) # index one less than spectrum number
   tof_ws = ConvertToPointData(tof_ws)
   SetInstrumentParameter(tof_ws, ParameterName='t0', ParameterType='Number', Value='0.5')
   SetInstrumentParameter(tof_ws, ParameterName='sigma_l1', ParameterType='Number', Value='0.021')
   SetInstrumentParameter(tof_ws, ParameterName='sigma_l2', ParameterType='Number', Value='0.023')
   SetInstrumentParameter(tof_ws, ParameterName='sigma_tof', ParameterType='Number', Value='0.3')
   SetInstrumentParameter(tof_ws, ParameterName='sigma_theta', ParameterType='Number', Value='0.028')
   SetInstrumentParameter(tof_ws, ParameterName='hwhm_lorentz', ParameterType='Number', Value='24.0')
   SetInstrumentParameter(tof_ws, ParameterName='sigma_gauss', ParameterType='Number', Value='73.0')
   ###########################################################################

   tof, ysp = VesuvioResolution(Workspace=tof_ws, Mass=1.0079)

   print('Resolution in %s and %s.' % (tof.getAxis(0).getUnit().symbol(), ysp.getAxis(0).getUnit().symbol()))

Output:

.. testoutput::

   Resolution in microsecond and A^-1.

.. categories::

.. sourcelink::
