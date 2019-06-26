.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm is the first step in the ILL reflectometry reduction workflow. It:

#. loads data from disk by using :ref:`LoadAndMerge <algm-LoadAndMerge>` (:ref:`LoadILLReflectometry <algm-LoadILLReflectometry>`, :ref:`MergeRuns <algm-MergeRuns>`)
#. merges the numors
#. determines the peak position by using :ref:`FindReflectometryLines <algm-FindReflectometryLines>`
#. moves the detector (of name *detector*) by using :ref:`SpecularReflectionPositionCorrect <algm-SpecularReflectionPositionCorrect>` (optional, if *DirectLineWorkspace* is given)
#. normalizes to a (water) reference (optional)
#. normalizes to slit sizes (optional)
#. normalizes to experiment time or monitor counts (optional)
#. subtracts time-independent background (optional)
#. converts to wavelength

The algorithm can be thought as an 'advanced loader', and should be used to load both direct beam and reflected beam measurements.

The *OutputWorkspace* can be further fed to :ref:`ReflectometryILLSumForeground <algm-ReflectometryILLSumForeground>`.

The algorithm adds the following sample log entries to the *OutputWorkspace*:

* reduction.foreground.centre_workspace_index
* reduction.foreground.last_workspace_index
* reduction.foreground.first_workspace_index
* reduction.line_position : the peak position (workspace index) used to define the :math:`2\theta` angles (detector positions)
* reduction.two_theta

The option *Slit Normalisation AUTO* will select the slit normalisation depending on the instrument: for D17 and FIGARO, the slit normalisation will be turned on and off, respectively.

The workflow diagram below gives an overview of the algorithm:

.. diagram:: ReflectometryILLPreprocess-v1_wkflw.dot

Detector angles
###############

A fitting of the present peak position takes place in order to determine the detector angles.
For preventing fitting of the present peak position, the property *LinePosition* allows to provide a peak position.
A use case is to enter a direct peak position, which can be obtained from the direct beam workspaces sample logs, when the *Run* is a reflected beam.

Alternatively, the properties *TwoTheta* and the sample log *reduction.line_position* affect the pixel :math:`2\theta` angle.

Foreground and backgrounds
##########################

Foreground is a set of pixels intensities of which will be summed in :ref:`ReflectometryILLSumForeground <algm-ReflectometryILLSumForeground>`. However, foreground needs to be defined already in this algorithm as the information is needed for the background pixels. The foreground pixel information is stored in the sample logs of *OutputWorkspace* under the entries starting with ``foreground.``.

Background, on the other hand, is a set of pixels which are be used for fitting a constant or linear background by :ref:`CalculatePolynomialBackground <algm-CalculatePolynomialBackground>`.

The foreground pixels are defined by the foreground centre and *ForegroundHalfWidth* property. In normal use cases, the foreground center (workspace index) is taken from the fitting in :ref:`LoadILLReflectometry <algm-LoadILLReflectometry>`. This can be overridden by giving the pixel as *BeamCentre*. Fractional values are rounded to nearest integer. The full process of deciding the foreground centre is as follows:

* If *Run* is given then data is loaded using :ref:`LoadILLReflectometry <algm-LoadILLReflectometry>`:
    * If *TwoTheta* is set, it is passed over to :ref:`SpecularReflectionPositionCorrect <algm-SpecularReflectionPositionCorrect>`.
    * Otherwise, the line position will be determined by peak fitting.
    * Use the beam centre returned by the :ref:`LoadILLReflectometry <algm-LoadILLReflectometry>`, rounded to nearest integer, as the foreground centre.
* If *InputWorkspace* is given:
    * If sample log entry `reduction.line_position` is given, round it to nearest integer and use as the foreground centre.
    * If *LinePosition* is given, round the value to nearest integer and use as the foreground centre.
    * Otherwise fit the beam centre using similar method to :ref:`LoadILLReflectometry <algm-LoadILLReflectometry>` and use the rounded result as the foreground centre.

*ForegroundHalfWidth* is a list of one or two values. If a single value is given, then this number of pixels on both sides of the centre pixel are included in the foreground. For example, ``ForegroundHalfWidth=[3]`` means three pixel on both sides are included, making the foreground seven pixels wide in total. ``ForegroundHalfWidth=[0]`` means that only the centre pixel is included. When two values are given, then the foreground is asymmetric around the centre. For instance, ``ForegroundHalfWidth[2,5]`` indicates that two pixel at lower :math:`\theta` and five pixels at higher :math:`\theta` are included in the foreground.

*LowAngleBkgWidth* and *HighAngleBkgWidth* define the number of the background fitting pixels at low and high :math:`\theta`. Either one or both widths can be defined. The distance between the background pixels and the foreground can in turn be given by *LowAngleBkgOffset* and *HighAngleBkgOffset*.

The following figure exemplifies the foreground and background for the D17 instrument at ILL. Note, that in this particular case, the pixel indices increase with decreasing :math:`\theta`.

.. plot::

   from mantid.api import mtd
   from mantid.simpleapi import ExtractMonitors, LoadILLReflectometry
   import matplotlib.pyplot as plt
   import numpy

   ws = LoadILLReflectometry('ILL/D17/317370.nxs')
   ExtractMonitors(ws, DetectorWorkspace='ws')
   ws=mtd['ws']
   det0 = ws.getDetector(0)
   det1 = ws.getDetector(ws.getNumberHistograms() - 1)
   theta0 = numpy.rad2deg(ws.detectorSignedTwoTheta(det0))
   theta1 = numpy.rad2deg(ws.detectorSignedTwoTheta(det1))
   fig, ax = plt.subplots(subplot_kw={'projection': 'mantid'})
   ax.pcolor(ws, cmap='Oranges')
   ax.set_xlim(xmin=3, xmax=27)
   ax.set_ylim(ymin=0, ymax=ws.getNumberHistograms())
   ax.set_ylabel('Pixel (workspace index)')
   ax.axhspan(238, 250, color='red', alpha=0.15)
   ax.text(4.5, 241, 'LowAngleBkgWidth')
   ax.text(5, 223, 'LowAngleBkgOffset')
   ax.axhspan(185, 215, color='blue', alpha=0.15)
   ax.text(5.5, 206, 'ForegroundWidth [0]')
   ax.axhline(203, linestyle=':', color='k')  # Line position
   ax.text(22, 200, 'LinePosition')
   ax.text(5.5, 190, 'ForegroundWidth [1]')
   ax.text(5, 162, 'HighAngleBkgOffset')
   ax.axhspan(75, 145, color='red', alpha=0.15)
   ax.text(4.5, 108, 'HighAngleBkgWidth')
   ax2 = ax.twinx()
   ax2.set_ylim(ymin=theta0, ymax=theta1)
   ax2.set_ylabel('Angle (degrees)')

InputWorkspace
##############

The *InputWorkspace* and *DirectBeamWorkspace* can be used instead of *Run* if the data is already loaded into Mantid for example using :ref:`LoadILLReflectometry <algm-LoadILLReflectometry>`. This option exists mainly for testing purposes.

Usage
-----

**Example - Load direct and reflected beams**

.. testcode:: ForegroundWidthsEx

   from directtools import SampleLogs

   # Use same foreground and background settings for direct and reflected
   # beams.
   # Python dictionaries can be passed to algorithms as 'keyword arguments'.
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

   reflected = ReflectometryILLPreprocess(
       Run='ILL/D17/317370.nxs',
       DirectLineWorkspace=direct,
       **settings
   )

   # Check foreground settings from sample logs
   logs = SampleLogs(reflected)
   print('Reflected line position: {}'.format(logs.reduction.foreground.centre_workspace_index))
   # Half widths + centre pixel
   width = logs.reduction.foreground.last_workspace_index - logs.reduction.foreground.first_workspace_index + 1
   print('Foreground width: {}'.format(width))

Output:

.. testoutput:: ForegroundWidthsEx

   Reflected line position: 202
   Foreground width: 11

.. categories::

.. sourcelink::
