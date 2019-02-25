.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm is typically the second step in the reflectometry reduction workflow. It consumes the output of :ref:`ReflectometryILLPreprocess <algm-ReflectometryILLPreprocess>`, producing a workspace with a single spectrum.

The reflectivity output of this algorithm can be forwarded to :ref:`ReflectometryILLConvertToQ <algm-ReflectometryILLConvertToQ>` or, in case of polarization analysis, :ref:`ReflectometryILLPolarizationCor <algm-ReflectometryILLPolarizationCor>`.

The following diagram gives an overview of the algorithm:

.. diagram:: ReflectometryILLSumForeground-v1_wkflw.dot

The algorihtm runs :ref:`ReflectometryBeamStatistics <algm-ReflectometryBeamStatistics>` when processing the reflected beam. This adds some sample log entries to *OutputWorkspace* and *DirectLineWorkspace*. See the :ref:`algorithm's documentation <algm-ReflectometryBeamStatistics>` for more details.

Summation type
##############

The *SummationType* property controls how the foreground pixels are summed.

*SumInLambda*
    extracts the centre pixel histogram using :ref:`ExtractSingleSpectrum <algm-ExtractSingleSpectrum>` and adds the intensities of the rest of the foreground pixels.
*SumInQ*
    sums the foreground pixels using :ref:`ReflectometrySumInQ <algm-ReflectometrySumInQ>`. Before summation, the data is divided by the direct beam.

The chosen *SummationType* will be added to the sample logs of *OutputWorkspace* under the ``foreground.summation_type`` entry.

Foreground pixels
#################

If *InputWorkspace* has been processed by :ref:`ReflectometryILLPreprocess <algm-ReflectometryILLPreprocess>`, the foreground information is available in the sample logs under the entries starting with ``foreground.``. By default these are used automatically.

The sample logs can be overridden using the *Foreground* property. It is a list of three integers defining the range and centre pixels as workspace indices: [start, centre, end]. The start and end values are inclusive.

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
       **settings
   )
   # We need the summed direct beam for the reflectivity
   directFgd = ReflectometryILLSumForeground(direct)

   # Reflected beam
   reflected = ReflectometryILLPreprocess(
       Run='ILL/D17/317370.nxs',
       DirectLineWorkspace=direct,
       **settings
   )
   reflectivity = ReflectometryILLSumForeground(
       InputWorkspace=reflected,
       DirectForegroundWorkspace=directFgd,
       DirectLineWorkspace=direct,
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
       **settings
   )

   # We need the summed direct beam for the reflectivity
   directFgd = ReflectometryILLSumForeground(direct)
   
   # Reflected beam
   reflected = ReflectometryILLPreprocess(
       Run='ILL/D17/317370.nxs',
       DirectLineWorkspace=direct,
       **settings
   )
   reflectivity = ReflectometryILLSumForeground(
       InputWorkspace=reflected,
       DirectForegroundWorkspace=directFgd,
       DirectLineWorkspace=direct,
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
