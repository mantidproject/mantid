.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm is typically the second step in the reflectometry reduction workflow. It consumes the output of :ref:`ReflectometryILLPreprocess <algm-ReflectometryILLPreprocess>`, producing

* the summed foreground if only *InputWorkspace* is given. This should be used for the direct beam.
* the reflectivity if both *InputWorkspace* and *DirectForegroundWorkspace* are given. *DirectForegroundWorkspace* should be the output of the above case, i.e. the summed direct foreground.

The reflectivity output of this algorithm can be forwarded to :ref:`ReflectometryILLConvertToQ <algm-ReflectometryILLConvertToQ>` or, in case of polarization analysis, :ref:`ReflectometryILLPolarizationCor <algm-ReflectometryILLPolarizationCor>`.

The following diagram gives an overview of the algorithm:

.. diagram:: ReflectometryILLSumForeground-v1_wkflw.dot

Summation type
##############

The *SummationType* property controls how the foreground pixels are summed.

*SumInLambda*
    extracts the centre pixel histogram using :ref:`ExtractSingleSpectrum <algm-ExtractSingleSpectrum>` and adds the intensities of the rest of the foreground pixels. Errors are summed in squares. The summed data is subsequently divided by the direct beam.
*SumInQ*
    sums the pixels using :ref:`ReflectometrySumInQ <algm-ReflectometrySumInQ>`. Before summation, the data is divided by the direct beam.

Foreground pixels
#################

If *InputWorkspace* has been processed by :ref:`ReflectometryILLPreprocess <algm-ReflectometryILLPreprocess>`, the foreground information is available in the sample logs under the entries starting with ``foreground.``. By default these are used automatically.

The sample logs can be overriden using the *Foreground* property. It is a list of three integers defining the range and centre pixels as workspace indices: [start, centre, end]. The start and end values are inclusive.

Usage
-----

.. include:: ../usagedata-note.txt

**Example - Sum in wavelength**

.. testcode:: ReflectivityExLambda

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
   # We need the summed direct beam for the reflectivity
   directFgd = ReflectometryILLSumForeground(direct.OutputWorkspace)

   # Reflected beam
   reflected = ReflectometryILLPreprocess(
       Run='ILL/D17/317370.nxs',
       DirectBeamPositionWorkspace='direct_beam_pos',
       **settings
   )
   reflectivity = ReflectometryILLSumForeground(
       InputWorkspace=reflected,
       DirectForegroundWorkspace=directFgd,
       WavelengthRange=[2, 15],
   )

   # Reflectivity is a single histogram
   print('Histograms in reflectivity workspace: {}'.format(reflectivity.getNumberHistograms()))
   # The data is still in wavelength
   print('Reflectivity X unit: ' + reflectivity.getAxis(0).getUnit().unitID())

Output:

.. testoutput:: ReflectivityExLambda

   Histograms in reflectivity workspace: 1
   Reflectivity X unit: Wavelength

**Example - Sum in momentum transfer**

.. testcode:: ReflectivityExQ

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
   # We need the summed direct beam for the reflectivity
   directFgd = ReflectometryILLSumForeground(direct.OutputWorkspace)

   # Reflected beam
   reflected = ReflectometryILLPreprocess(
       Run='ILL/D17/317370.nxs',
       DirectBeamPositionWorkspace='direct_beam_pos',
       **settings
   )
   reflectivity = ReflectometryILLSumForeground(
       InputWorkspace=reflected,
       DirectForegroundWorkspace=directFgd,
       SummationType='SumInQ',
       WavelengthRange=[0., 14.]
   )

   # Reflectivity is a single histogram
   print('Histograms in reflectivity workspace: {}'.format(reflectivity.getNumberHistograms()))
   # The data is still in wavelength
   print('Reflectivity X unit: ' + reflectivity.getAxis(0).getUnit().unitID())

Output:

.. testoutput:: ReflectivityExQ

   Histograms in reflectivity workspace: 1
   Reflectivity X unit: Wavelength

.. categories::

.. sourcelink::
