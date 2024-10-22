
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Performs the post fitting corrections steps for Vesuvio data.
These steps in include corrections for multiple scattering and gamma.

Usage
-----

**Example - VesuvioCorrections**

.. testcode:: VesuvioCorrectionsExample

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

  #######################Create dummy fit parameters#############################
  params = CreateEmptyTableWorkspace(OutputWorkspace='__VesuvioCorrections_test_fit_params')

  params.addColumn('str', 'Name')
  params.addColumn('float', 'Value')
  params.addColumn('float', 'Error')

  params.addRow(['f0.Width', 4.72912, 0.41472])
  params.addRow(['f0.FSECoeff', 0.557332, 0])
  params.addRow(['f0.C_0', 11.8336, 1.11468])
  params.addRow(['f1.Width', 10, 0])
  params.addRow(['f1.Intensity', 2.21085, 0.481347])
  params.addRow(['f2.Width', 13, 0])
  params.addRow(['f2.Intensity', 1.42443, 0.583283])
  params.addRow(['f3.Width', 30, 0])
  params.addRow(['f3.Intensity', 0.499497, 0.28436])
  params.addRow(['f4.A0', -0.00278903, 0.00266163])
  params.addRow(['f4.A1', 14.5313, 22.2307])
  params.addRow(['f4.A2', -5475.01, 35984.4])
  params.addRow(['Cost function value', 2.34392, 0])


  masses = [1.0079, 16.0, 27.0, 133.0]
  profiles = "function=GramCharlier,hermite_coeffs=[1, 0, 0],k_free=0,sears_flag=1,width=[2, 5, 7];function=Gaussian,width=10;function=Gaussian,width=13;function=Gaussian,width=30"



  corrections, corrected, linear_fit, out_ws = VesuvioCorrections(InputWorkspace=tof_ws,
                                                                  GammaBackground=True,
                                                                  FitParameters=params,
                                                                  Masses=masses,
                                                                  MassProfiles=profiles,
                                                                  ContainerScale=0.1,
                                                                  GammaBackgroundScale=0.2)

.. categories::

.. sourcelink::
