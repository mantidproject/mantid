
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Runs a Time of Flight (TOF) fit for preprocessed Vesuvio data.
This algorithm also constructs the constraints, ties, function strings for the fit based on the inputs.

Usage
-----

**Example - VesuvioTOFFit**

.. testcode:: VesuvioTOFFitExample

   ###### Simulates LoadVesuvio with spectrum number 135-136 #################
   tof_ws = CreateSimulationWorkspace(Instrument='Vesuvio',BinParams=[50,0.5,562],UnitX='TOF')
   tof_ws = CropWorkspace(tof_ws,StartWorkspaceIndex=134,EndWorkspaceIndex=135) # index one less than spectrum number
   tof_ws = ConvertToPointData(tof_ws)
   SetInstrumentParameter(tof_ws, ParameterName='t0',ParameterType='Number',Value='0.5')
   SetInstrumentParameter(tof_ws, ParameterName='sigma_l1', ParameterType='Number', Value='0.021')
   SetInstrumentParameter(tof_ws, ParameterName='sigma_l2', ParameterType='Number', Value='0.023')
   SetInstrumentParameter(tof_ws, ParameterName='sigma_tof', ParameterType='Number', Value='0.3')
   SetInstrumentParameter(tof_ws, ParameterName='sigma_theta', ParameterType='Number', Value='0.028')
   SetInstrumentParameter(tof_ws, ParameterName='hwhm_lorentz', ParameterType='Number', Value='24.0')
   SetInstrumentParameter(tof_ws, ParameterName='sigma_gauss', ParameterType='Number', Value='73.0')
   # Algorithm allows separate parameters for the foils
   SetInstrumentParameter(tof_ws, ComponentName='foil-pos0', ParameterName='hwhm_lorentz',
                          ParameterType='Number', Value='144.0')
   SetInstrumentParameter(tof_ws, ComponentName='foil-pos0', ParameterName='sigma_gauss',
                          ParameterType='Number', Value='20.0')
   SetInstrumentParameter(tof_ws, ComponentName='foil-pos1', ParameterName='hwhm_lorentz',
                          ParameterType='Number', Value='144.0')
   SetInstrumentParameter(tof_ws, ComponentName='foil-pos1', ParameterName='sigma_gauss',
                          ParameterType='Number', Value='20.0')

   ###########################################################################

   profiles = "function=GramCharlier,width=[2, 5, 7],hermite_coeffs=[1, 0, 0],k_free=0,sears_flag=1;"\
              "function=Gaussian,width=10;function=Gaussian,width=13;function=Gaussian,width=30;"
   background = "function=Polynomial,order=3"

   out_ws, fit_params, chi_sq = VesuvioTOFFit(InputWorkspace=tof_ws, Masses=[1.0079, 16.0, 27.0, 133.0],
                                              MassProfiles=profiles, Background=background,
                                              IntensityConstraints='[0,1,0,-4]')

.. categories::

.. sourcelink::
