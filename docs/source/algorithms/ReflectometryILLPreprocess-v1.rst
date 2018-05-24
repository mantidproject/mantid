.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm is the first step in the ILL reflectometry reduction workflow. It:

#. loads data from disk
#. merges the numors
#. normalizes to a (water) reference
#. normalizes to slit sizes
#. normalizes to experiment time or monitor counts
#. subtracts time-independent background
#. crop to given wavelength range (in TOF)
#. converts to wavelength

The algorithm can be thought as an 'advanced loader', and should be used to load both direct beam and reflected beam measurements.

The *OutputWorkspace* can be further fed to :ref:`ReflectometryILLSumForeground <algm-ReflectometryILLSumForeground>`.

The workflow diagram below gives an overview of the algorithm:

.. diagram:: ReflectometryILLPreprocess-v1_wkflw.dot

Detector angles
###############

The properties *BeamCentre*, *BraggAngle* and *DirectBeamPositionWorkspace* affect the pixel :math:`\theta` angles. They map directly to the corresponding properties of :ref:`LoadILLReflectometry <algm-LoadILLReflectometry>`. The *BeamCentre* property can be further used in determining the foreground pixels as discussed below.

Foreground and backgrounds
##########################

Foreground is a set of pixels intensities of which will be summed in :ref:`ReflectometryILLSumForeground <algm-ReflectometryILLSumForeground>`. However, foreground needs to be defined already in this algorithm as the information is needed for the background pixels. The foreground pixel information is stored in the sample logs of *OutputWorkspace* under the entries starting with ``foreground.``.

Background, on the other hand, is a set of pixels which are be used for fitting a constant or linear background by :ref:`CalculatePolynomialBackground <algm-CalculatePolynomialBackground>`.

The foreground pixels are defined by the foreground centre and *ForegroundHalfWidth* property. In normal use cases, the foreground center (workspace index) is taken from the fitting in :ref:`LoadILLReflectometry <algm-LoadILLReflectometry>`. This can be overriden by giving the pixel as *BeamCentre*. Fractional values are rounded to nearest integer. The full process of deciding the foreground centre is as follows:

* If *Run* is given then data is loaded using :ref:`LoadILLReflectometry <algm-LoadILLReflectometry>`:
    * If *BeamCentre* is set, it is passed over to :ref:`LoadILLReflectometry <algm-LoadILLReflectometry>`.
    * Otherwise, :ref:`LoadILLReflectometry <algm-LoadILLReflectometry>` will fit the beam centre.
    * Use the beam centre returned by the :ref:`LoadILLReflectometry <algm-LoadILLReflectometry>`, rounded to nearest integer, as the foreground centre.
* If *InputWorkspace* is given:
    * If *BeamPositionWorkspace* is given, take the beam centre from there, round it to nearest integer and use as the foreground centre.
    * If *BeamCentre* is given, round the value to nearest integer and use as the foreground centre.
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
   ax.axhline(203, linestyle=':', color='k')  # Beam centre
   ax.text(22, 200, 'BeamCentre')
   ax.text(5.5, 190, 'ForegroundWidth [1]')
   ax.text(5, 162, 'HighAngleBkgOffset')
   ax.axhspan(75, 145, color='red', alpha=0.15)
   ax.text(4.5, 108, 'HighAngleBkgWidth')
   ax2 = ax.twinx()
   ax2.set_ylim(ymin=theta0, ymax=theta1)
   ax2.set_ylabel('Angle (degrees)')

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
       OutputBeamPositionWorkspace='direct_beam_pos',  # For reflected angle calibration.
       **settings
   )

   reflected = ReflectometryILLPreprocess(
       Run='ILL/D17/317370.nxs',
       DirectBeamPositionWorkspace='direct_beam_pos',
       **settings
   )

   # Check foreground settings from sample logs
   logs = SampleLogs(reflected)
   print('Reflected beam centre: {}'.format(logs.foreground.centre_workspace_index))
   # Half widths + centre pixel
   width = logs.foreground.last_workspace_index - logs.foreground.first_workspace_index + 1
   print('Foreground width: {}'.format(width))

Output:

.. testoutput:: ForegroundWidthsEx

   Reflected beam centre: 202
   Foreground width: 11

.. categories::

.. sourcelink::
