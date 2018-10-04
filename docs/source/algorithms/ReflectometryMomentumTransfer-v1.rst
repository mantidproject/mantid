.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm converts a reflectivity workspace from wavelength to momentum transfer :math:`Q_{z}` and calculates the :math:`Q_{z}` resolution. The resolution is added as the Dx (X Errors) field in the output workspace. This algorithms processes workspaces in which the pixels containing the reflectected line have been integrated into a single histogram. For conversion of a 2D workspace into :math:`Q_{x}, Q_{z}` or equivalent momentum space, see :ref:`ConvertToReflectometryQ <algm-ConvertToReflectometryQ>`.

The two additional input workspaces, *ReflectedBeamWorkspace* and *DirectBeamWorkspace* are the raw reflected and direct beam workspaces before foreground summation. They are needed for the resolution calculation.

The instruments of all three input workspaces are expected contain two components representing the two slits in the beam before the sample. The names of these components are given to the algorithm as the *FirstSlitName* and *SecondSlitName* properties. The slit openings (width or height depending on reflectometer setup) should be written in the sample logs (units 'm' or 'mm'). The log enties are named by *FirstSlitSizeSampleLog* and *SecondSlitSizeSampleLog*.

The *Polarized* property should be used to indicate whether *InputWorkspace* is part of a polarization analysis dataset.

The *SummationType* property reflects the type of foreground summation used to obtain the reflectivity workspace.

Conversion to momentum transfer
###############################

The unit conversion from wavelength to :math:`Q_{z}` is done by :ref:`ConvertUnits <algm-ConvertUnits>`.

:math:`Q_{z}` resolution
########################

The resolution calculation follows the procedure described in [#Gutfreund]_.

Usage
-----

.. include:: ../usagedata-note.txt

**Example - ReflectometryMomentumTransfer**

.. testcode:: ReflectometryMomentumTransferExample

   # Load data.
   reflectedWS = LoadILLReflectometry('ILL/D17/317370.nxs', XUnit='TimeOfFlight')
   ConvertToDistribution(reflectedWS)
   directWS = LoadILLReflectometry('ILL/D17/317369.nxs', XUnit='TimeOfFlight')
   ConvertToDistribution(directWS)

   # Extract some instrument parameters.
   chopperPairDistance = 1e-2 * reflectedWS.run().getProperty('Distance.ChopperGap').value
   chopperSpeed = reflectedWS.run().getProperty('VirtualChopper.chopper1_speed_average').value
   chopper1Phase = reflectedWS.run().getProperty('VirtualChopper.chopper1_phase_average').value
   chopper2Phase = reflectedWS.run().getProperty('VirtualChopper.chopper2_phase_average').value
   openoffset = reflectedWS.run().getProperty('VirtualChopper.open_offset').value

   # Normalize to time.
   duration = reflectedWS.run().getProperty('duration').value
   reflectedWS /= duration
   duration = directWS.run().getProperty('duration').value
   directWS /= duration

   # Calculate reflectivity.
   refForeground = SumSpectra(reflectedWS, 198, 209)
   dirForeground = SumSpectra(directWS, 190, 210)
   refForeground = RebinToWorkspace(WorkspaceToRebin=refForeground, WorkspaceToMatch=dirForeground)
   R = refForeground / dirForeground

   # Convert TOF to wavelength, crop.
   R = ConvertUnits(R, 'Wavelength')
   R = CropWorkspace(R, XMin=4.3, XMax=14.0, StoreInADS=False)
   n = reflectedWS.getNumberHistograms()
   reflectedWS = ConvertUnits(reflectedWS, 'Wavelength')
   reflectedWS = CropWorkspaceRagged(reflectedWS, XMin=n*[4.3], XMax=n*[14.0], StoreInADS=False)
   directWS = ConvertUnits(directWS, 'Wavelength')
   directWS = CropWorkspaceRagged(directWS, XMin=n*[4.3], XMax=n*[14.0])

   outws = ReflectometryMomentumTransfer(
       R,
       reflectedWS,
       directWS,
       ReflectedForeground=[198, 209],
       DirectForeground=[190, 210],
       SummationType='SumInLambda',
       Polarized=False,
       PixelSize=0.001195,
       DetectorResolution=0.0022,
       ChopperRadius=0.36,
       ChopperSpeed=chopperSpeed,
       ChopperOpening=45. - (chopper2Phase - chopper1Phase) - openoffset,
       ChopperPairDistance=chopperPairDistance,
       FirstSlitName='slit2',
       FirstSlitSizeSampleLog='VirtualSlitAxis.s2w_actual_width',
       SecondSlitName='slit3',
       SecondSlitSizeSampleLog='VirtualSlitAxis.s3w_actual_width',
       TOFChannelWidth=57.
   )

   qs = outws.readX(0)
   dqs = outws.readDx(0)
   print('First refectivity point Qz = {:.4f} +- {:.4f} A-1'.format(qs[0], dqs[0]))
   print('and last Qz = {:.4f} +- {:.4f} A-1'.format(qs[-1], dqs[-1]))

Output:

.. testoutput:: ReflectometryMomentumTransferExample

   First refectivity point Qz = 0.0118 +- 0.0001 A-1
   and last Qz = 0.0381 +- 0.0005 A-1

References
----------

.. [#Gutfreund] P. Gutfreund, T. Saerbeck, M. A. Gonzalez, E. Pellegrini, M. Laver, C. Dewhurst, R. Cubitt,
             `arXiv:1710.04139  <https://arxiv.org/abs/1710.04139>`_ **\[physics.ins-det\]**

.. categories::

.. sourcelink::

