.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm is currently designed to be used by the Vesuvio spectrometer at ISIS to
correct for background produced by photons that are created when the
neutrons are absorbed by the shielding on the instrument. It only
corrects the forward scattering detector banks - spectrum numbers 135-198.

Two workspaces are produced: the calculated background and a corrected
workspace where the input workspace has been corrected by the
background. The background is computed by a simple simulation of the
expected count across all of the foils. The corrected workspace counts
are computed by calculating a ratio of the expected counts at the
detector to the integrated foil counts (:math:`\beta`) and then the
final corrected count rate :math:`\displaystyle c_f` is defined as
:math:`\displaystyle c_f = c_i - \beta c_b`.

Usage
-----

**Example: Correcting the first spectrum**

.. testcode::

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

   mass_function = "name=GaussianComptonProfile,Mass=1.0079,Width=0.4,Intensity=1.1"
   corrected, background = VesuvioCalculateGammaBackground(tof_ws, ComptonFunction=mass_function,
                                                    WorkspaceIndexList=0)

   print("First 5 values of input: {}".format(tof_ws.readY(0)[0:4]))
   print("First 5 values of background: {}".format(background.readY(0)[0:4]))
   print("First 5 values of corrected: {}".format(corrected.readY(0)[0:4]))

Output:

.. testoutput::

   First 5 values of input: [1. 1. 1. 1.]
   First 5 values of background: [1.00053363 1.00054706 1.00056074 1.0005747 ]
   First 5 values of corrected: [-0.00053363 -0.00054706 -0.00056074 -0.0005747 ]

**Example: Correcting all spectra**

.. testcode::

   ###### Simulates LoadVesuvio with spectrum number 135-136 #################
   tof_ws = CreateSimulationWorkspace(Instrument='Vesuvio',BinParams=[50,0.5,562],UnitX='TOF')
   tof_ws = CropWorkspace(tof_ws,StartWorkspaceIndex=134,EndWorkspaceIndex=135) # index one less than spectrum number5
   tof_ws = ConvertToPointData(tof_ws)
   SetInstrumentParameter(tof_ws, ParameterName='t0',ParameterType='Number',Value='0.5')
   SetInstrumentParameter(tof_ws, ParameterName='sigma_l1', ParameterType='Number', Value='0.021')
   SetInstrumentParameter(tof_ws, ParameterName='sigma_l2', ParameterType='Number', Value='0.023')
   SetInstrumentParameter(tof_ws, ParameterName='sigma_tof', ParameterType='Number', Value='0.3')
   SetInstrumentParameter(tof_ws, ParameterName='sigma_theta', ParameterType='Number', Value='0.028')
   SetInstrumentParameter(tof_ws, ParameterName='hwhm_lorentz', ParameterType='Number', Value='73.0')
   SetInstrumentParameter(tof_ws, ParameterName='sigma_gauss', ParameterType='Number', Value='24.0')
   ###########################################################################

   mass_function = "name=GaussianComptonProfile,Mass=1.0079,Width=0.4,Intensity=1.1"
   corrected, background = VesuvioCalculateGammaBackground(tof_ws, ComptonFunction=mass_function)

   print("Number of background spectra: {}".format(background.getNumberHistograms()))
   print("Number of corrected spectra: {}".format(corrected.getNumberHistograms()))

Output:

.. testoutput::

   Number of background spectra: 2
   Number of corrected spectra: 2

.. categories::

.. sourcelink::
