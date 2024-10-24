
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm computes quantities needed by :ref:`ReflectometryMomentumTransfer <algm-ReflectometryMomentumTransfer>` and :ref:`ReflectometrySumInQ <algm-ReflectometrySumInQ>`, and adds the results to the sample logs of *ReflectedBeamWorkspace*. The following sample logs get added:

``beam_stats.beam_rms_variation``
   :math:`=2 \sqrt{2 \ln 2} s \sqrt{\sigma}`, where :math:`s` is *PixelSize* and :math:`\sigma` is the variance of the intensity (integrated over all wavelengths) distribution of the detectors in the foreground region.

``beam_stats.bent_sample``
   1 if the sample can be regarded as non-flat and the beam is collimated, 0 in the case of divergent beam.

``bean_stats.first_slit_angular_spread``
   :math:`=0.68 x_{slit1} / d_{slits}`, where :math:`x_{slit1}` is the size of the first slit and :math:`d_{slits}` is the distance between the first and second slit.

``beam_stats.incident_angular_spread``
   :math:`=0.68 \sqrt{x_{slit1}^2 + x_{slit2}^2} / d_{slits}`, where :math:`x_{slit1}` is the size of the first and :math:`x_{slit2}` the size of the second slit while :math:`d_{slits}` is the distance between the slits.

``beam_stats.sample_waviness``
   The heuristically calculated root mean squared sample waviness.

``beam_stats.second_slit_angular_spread``
   :math:`=0.68 x_{slit2} / (d_{slit2} + l_2)`, where :math:`x_{slit2}` is the size of the second slit, :math:`d_{slit2}` is the second slit-to-sample distance and :math:`l_2` is the sample-to-reflected foreground centre distance.

Additionally, ``beam_stats.beam_rms_variation`` is cached to the sample logs of *DirectLineWorkspace* removing the need to recalculate the quantity every time the same direct beam passed to this algorithm.

Usage
-----
..  Try not to use files in your examples,
    but if you cannot avoid it then the (small) files must be added to
    autotestdata\UsageData and the following tag unindented
    .. include:: ../usagedata-note.txt

**Example - ReflectometryBeamStatistics**

.. testcode:: ReflectometryBeamStatisticsExample

   dir = Load('ILL/D17/317369.nxs')
   ref = Load('ILL/D17/317370.nxs')

   ReflectometryBeamStatistics(
       ReflectedBeamWorkspace=ref,
       ReflectedForeground=[199, 202, 205],
       DirectLineWorkspace=dir,
       DirectForeground=[200, 202, 205],
       PixelSize=0.001195,
       DetectorResolution=0.00022,
       FirstSlitName='slit2',
       FirstSlitSizeSampleLog='VirtualSlitAxis.s2w_actual_width',
       SecondSlitName='slit3',
       SecondSlitSizeSampleLog='VirtualSlitAxis.s3w_actual_width')
   run = ref.run()
   bent = run.getProperty('beam_stats.bent_sample').value
   print('Bent sample? {}'.format('yes' if bent == 1 else 'no'))
   rms = run.getProperty('beam_stats.beam_rms_variation').value
   print('Beam RMS variation: {:.3}'.format(rms))
   run = dir.run()
   rms = run.getProperty('beam_stats.beam_rms_variation').value
   print('RMS variation cached in dir: {:.3}'.format(rms))

Output:

.. testoutput:: ReflectometryBeamStatisticsExample

   Bent sample? no
   Beam RMS variation: 0.00236
   RMS variation cached in dir: 0.00208

.. categories::

.. sourcelink::
