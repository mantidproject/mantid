
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Applys the preprocessing steps to loaded vesuvio data before it is fitted.
These steps include:
- Smoothing the data
- Masking bad detectors 

Usage
-----

**Example - VesuvioPreFit**

.. testcode:: VesuvioPreFitExample

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

    pre_fit_workspace = VesuvioPreFit(Inputworkspace=tof_ws, Smoothing="Neighbour",
                                      SmoothingOptions="NPoints=3", BadDataError=-1)

.. categories::

.. sourcelink::

