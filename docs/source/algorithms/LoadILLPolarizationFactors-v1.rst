
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm loads reflectometry polarization efficiency data from ILL's IDL-style text files. The output is a workspace with the efficiency factors calculated for each bin or point in the first spectrum of the *WavelengthReference* workspace. The output workspace is populated with five histograms carrying the following vertical axis labels:

:literal:`P1`
   the probability of neutrons not being correctly polarized

:literal:`P2`
   the probability of neutrons not being correctly analyzed

:literal:`F1`
   the probability of neutron polarization being flipped at the pre-sample flipper

:literal:`F2`
   the probability of neutron polarization being flipped at the post-sample flipper

:literal:`Phi`
   the combined polarizer-analyzer efficiencies
 
Note, that the order of the histograms in the output workspace is unspecified. The histograms should be identified by the above labels instead.

The errors in the output histograms are estimated by multiplying the efficiencies by constant factors.

Usage
-----

.. include:: ../usagedata-note.txt

**Example - Loading sample data and corresponding efficiencies**

.. testcode:: LoadILLPolarizationFactorsExample

   # We need reference wavelengths. Lets use some real data for that.
   # Load direct beam for detector angle calibration.
   db = LoadILLReflectometry(Filename='ILL/D17/317369.nxs',
                             OutputBeamPosition='direct_beam_pos',
                             XUnit='TimeOfFlight')
   rb = LoadILLReflectometry(Filename='ILL/D17/317370.nxs',
                             DirectBeamPosition='direct_beam_pos',
                             XUnit='TimeOfFlight')
   # Sum over the reflected beam.
   rb_grouped = GroupDetectors(rb, GroupingPattern='201-203')
   rb_in_wavelength = ConvertUnits(rb_grouped, Target='Wavelength')
   rb_cropped = CropWorkspace(rb_in_wavelength, XMin=0)
   pol_factors = LoadILLPolarizationFactors(Filename='ILL/D17/PolarizationFactors.txt',
                                            WavelengthReference=rb_cropped)
   # Grab the vertical axis.
   axis = pol_factors.getAxis(1)
   axisSize = axis.length()
   factors = []
   for i in range(axisSize):
       factors.append(axis.label(i))
   print("The following factors were loaded from the file: {}".format(factors))

Output:

.. testoutput:: LoadILLPolarizationFactorsExample

   The following factors were loaded from the file: ['F1', 'F2', 'P1', 'P2', 'Phi']

.. categories::

.. sourcelink::

