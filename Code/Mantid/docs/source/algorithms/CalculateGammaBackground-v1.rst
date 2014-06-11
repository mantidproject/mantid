.. algorithm::

.. summary::

.. alias::

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
   ###########################################################################

   mass_function = "name=GaussianComptonProfile,Mass=1.0079,Width=0.4,Intensity=1.1"
   corrected, background = CalculateGammaBackground(tof_ws, ComptonFunction=mass_function,
                                                    WorkspaceIndexList=0)

   print "First 5 values of input:", tof_ws.readY(0)[0:4]
   print "First 5 values of background:", background.readY(0)[0:4]
   print "First 5 values of corrected:", corrected.readY(0)[0:4]

Output:

.. testoutput::

   First 5 values of input: [ 1.  1.  1.  1.]
   First 5 values of background: [ 1.00053361  1.00054704  1.00056072  1.00057468]
   First 5 values of corrected: [-0.00053361 -0.00054704 -0.00056072 -0.00057468]

**Example: Correcting all spectra**

.. testcode::

   ###### Simulates LoadVesuvio with spectrum number 135-136 #################
   tof_ws = CreateSimulationWorkspace(Instrument='Vesuvio',BinParams=[50,0.5,562],UnitX='TOF')
   tof_ws = CropWorkspace(tof_ws,StartWorkspaceIndex=134,EndWorkspaceIndex=135) # index one less than spectrum number5
   tof_ws = ConvertToPointData(tof_ws)
   SetInstrumentParameter(tof_ws, ParameterName='t0',ParameterType='Number',Value='0.5')
   ###########################################################################

   mass_function = "name=GaussianComptonProfile,Mass=1.0079,Width=0.4,Intensity=1.1"
   corrected, background = CalculateGammaBackground(tof_ws, ComptonFunction=mass_function)

   print "Number of background spectra:", background.getNumberHistograms()
   print "Number of corrected spectra:", corrected.getNumberHistograms()

Output:

.. testoutput::

   Number of background spectra: 2
   Number of corrected spectra: 2

.. categories::
