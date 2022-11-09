.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm is the first step in the ILL reflectometry reduction workflow. It:

#. loads data from disk by using :ref:`LoadAndMerge <algm-LoadAndMerge>` (:ref:`LoadILLReflectometry <algm-LoadILLReflectometry>`, :ref:`MergeRuns <algm-MergeRuns>`)
#. merges the numors
#. performs the detector angle calibration
#. normalizes to a (water) reference (optional)
#. normalizes to slit sizes (optional)
#. normalizes to experiment time or monitor counts (optional)
#. subtracts time-independent background (optional)
#. converts to wavelength
#. calculates gravity correction to wavelength and reflected angle (optional)

The algorithm can be thought as an 'advanced loader', and should be used to load both direct beam and reflected beam measurements.

The *OutputWorkspace* can be further fed to :ref:`ReflectometryILLSumForeground <algm-ReflectometryILLSumForeground>`.

The option *Slit Normalisation AUTO* will select the slit normalisation depending on the instrument: for D17 and FIGARO, the slit normalisation will be turned on and off, respectively.

The workflow diagram below gives an overview of the algorithm for direct and reflected beam preprocessing respectively:

.. diagram:: ReflectometryILLPreprocessDirect-v1_wkflw.dot

.. diagram:: ReflectometryILLPreprocessReflected-v1_wkflw.dot

Measurement
###########

This tag defines whether the runs need to be processed as direct or reflected beams.
In both cases the fractional foreground centre is fitted.
For direct beam, the detector is rotated such that the foreground centre corresponds exactly to zero scattering angle.
For reflected beam there are three possibilities, depending on the **AngleOption**:

#. **UserAngle**: the detector is rotated such that the foreground centre of the reflected beam corresponds exactly to **2*BraggAngle**.
#. **SampleAngle**: the detector is rotated such that the foreground centre if the reflected beam corresponds exactly to **2*SAN**, where the SAN is read from the nexus file of the reflected beam run.
#. **DetectorAngle**: the detector is rotated such that the foreground centre of the corresponding direct beam corresponds exactly to **DAN_RB - DAN_DB**, where the two detector angles are read from the direct and reflected beams correspondingly. In this case the **DirectBeamDetectorAngle** and **DirectBeamForegroundCentre** are required, which could be retrieved from the sample logs of the pre-processed direct beam runs.

Foreground and backgrounds
##########################

Foreground is a set of pixels intensities of which will be summed in :ref:`ReflectometryILLSumForeground <algm-ReflectometryILLSumForeground>`. However, foreground needs to be defined already in this algorithm as the information is needed for the background pixels. The foreground pixel information is stored in the sample logs of *OutputWorkspace* under the entries starting with ``foreground.``.

Background, on the other hand, is a set of pixels which are be used for average (default), fitted constant or linear background by :ref:`CalculatePolynomialBackground <algm-CalculatePolynomialBackground>`.

The foreground pixels are defined by the foreground centre and *ForegroundHalfWidth* property. In normal use cases, the foreground center (workspace index) is taken from the fitting in :ref:`LoadILLReflectometry <algm-LoadILLReflectometry>`. Fractional values are rounded to nearest integer.

*ForegroundHalfWidth* is a list of one or two values. If a single value is given, then this number of pixels on both sides of the centre pixel are included in the foreground. For example, ``ForegroundHalfWidth=[3]`` means three pixel on both sides are included, making the foreground seven pixels wide in total.
``ForegroundHalfWidth=[0]`` means that only the centre pixel is included. When two values are given, then the foreground is asymmetric around the centre. For instance, ``ForegroundHalfWidth[2,5]`` indicates that two pixel at lower :math:`\theta` and five pixels at higher :math:`\theta` are included in the foreground.

*LowAngleBkgWidth* and *HighAngleBkgWidth* define the number of the background fitting pixels at low and high :math:`\theta`. Either one or both widths can be defined. The distance between the background pixels and the foreground can in turn be given by *LowAngleBkgOffset* and *HighAngleBkgOffset*.

Usage
-----

**Example - Load direct and reflected beams with DAN calibration**

.. testcode:: D17Dan

   settings = {
       'ForegroundHalfWidth':[5],
       'LowAngleBkgOffset': 10,
       'LowAngleBkgWidth': 20,
       'HighAngleBkgOffset': 10,
       'HighAngleBkgWidth': 50
   }

   direct = ReflectometryILLPreprocess(
       Run='ILL/D17/317369.nxs',
       **settings
   )

   db_fg_centre = direct.run().getLogData('reduction.line_position').value
   db_dan = direct.run().getLogData('DAN.value').value

   reflected = ReflectometryILLPreprocess(
       Run='ILL/D17/317370.nxs',
       Measurement='ReflectedBeam',
       AngleOption='DetectorAngle',
       DirectBeamForegroundCentre=db_fg_centre,
       DirectBeamDetectorAngle=db_dan,
       **settings
   )

   rb_fg_centre = reflected.run().getLogData('reduction.line_position').value
   rb_dan = reflected.run().getLogData('DAN.value').value

   print('Reflected line position: {}'.format(int(rb_fg_centre)))

Output:

.. testoutput:: D17Dan

   Reflected line position: 201

.. categories::

.. sourcelink::
