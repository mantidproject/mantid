.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm executes the full data reduction for ILL reflectometers D17 and FIGARO in TOF mode (specular reflection) following [#Gutfreund]_.

Input
-----

The mandatory inputs are comma separated list of nexus files for direct and reflected beam measurements.
**,** stands as the separator of different angle configurations.
**+** (sum) or **-** (range sum) operations can be used to sum different files at the same instrument configuration.
When summing the metadata (e.g. acquisition time) will also be summed, so that the subsequent normalisation is handled correctly.
There must be the same number of angle configurations both for direct and reflected beam inputs.

Output
------

The output is a workspace group that contains the calculated reflectivity curves as a function of the momentum transfer.
The output is point data and has the calculated Q resolution attributed to it.
There is a separate output for each angle configuration.
An automatically stitched result is also produced.
Stitch in this case just takes the union of all the initial points without merging or removal, only scaling can be applied.
The outputs can be readily saved by :ref:`SaveReflectometryAscii <algm-SaveReflectometryAscii>` algorithm for further analysis.

Bragg Angle
-----------

If user specified :math:`\theta` angles are provided, they will be used.
Otherwise `SampleAngle` or `DetectorAngle` (default) option is executed.

Summation Type
--------------

The default summation type is incoherent (sum along constant :math:`\lambda`), where the reflectivity curve is calculated by dividing the summed foreground of the reflected beam by the summed foreground of the direct beam.
For coherent summing, first the reflected beam data is divided by the direct beam data in 2D, then the ratio is summed along the lines of the constant :math:`Q_{z}`.

Options
-------

Many options can be specified as a single value, which will be applied to all the angle configurations, or as a list of values.
In the case of the latter, the list must be of the same size, as many different angle configurations there are.

Direct Beam Caching
-------------------

The processed direct beam runs can be cached in Analysis Data Service in order to save significant time when multiple samples correspond to the same direct beam with the same processing configurations.
The name for the cached direct beam runs are derived from the run numbers; it is the run number if there is only one numor, or the first run number if there are several numors summed.
Both the processed direct beam, and its summed foreground are cached.
Care must be taken when enabling the caching, since if the same direct beam must be used with different options (e.g. different wavelength ranges) for different reflected beams, the caching will result in an error in subsequent reduction steps due to incompatibility of the workspaces.
In such case the caching must be disabled, or one has to clean the cache manually by deleting the corresponding workspaces.

Gravity correction
------------------

Gravity correction is relevant for data reduction at FIGARO. Its execution is steered by a switch: `CorrectGravity`. The correction follows the algorithm described in Ref. [#Gutfreund]_, and the application to data is split in two separate steps. The first step corrects the wavelength axis of both direct and reflected beams, and takes place in :ref:`ReflectometryILLPreprocess <algm-ReflectometryILLPreprocess>`, where also the corrected by gravity reflection angle is calculated. The second and final step corrects the reflection angle, and is applied to data in :ref:`ReflectometryILLConvertToQ <algm-ReflectometryILLConvertToQ>`.

Replacing sample logs
---------------------

It is possible to replace any sample log of the loaded data, or add a new log, using the `LogsToReplace` property. The key-value pairs must be provided as JSON-compatible strings or Python
dictionaries. For an example, see :ref:`LoadILLReflectometry <algm-LoadILLReflectometry>` usage.

Workflow
--------

.. diagram:: ReflectometryILLAutoProcess-v1_wkflw.dot

Usage
-----

.. include:: ../usagedata-note.txt

**Example - Single Angle**

.. testsetup:: SingleAngle

    config['default.facility'] = 'ILL'
    config['default.instrument'] = 'D17'
    config.appendDataSearchSubDir('ILL/D17/')

.. testcode:: SingleAngle

    ws = ReflectometryILLAutoProcess(
     Run='317370',
     DirectRun='317369',
     WavelengthLowerBound=3.5,
     WavelengthUpperBound=24.5,
     DeltaQFractionBinning=0.1
    )
    print('The R(Q) workspace has {0} points'.format(ws.getItem(0).blocksize()))

.. testoutput:: SingleAngle

    The R(Q) workspace has 672 points

.. testcleanup:: SingleAngle

    import os
    os.remove("ws_0.out")

**Example - Multiple Angles**

.. testsetup:: MultipleAngles

    config['default.facility'] = 'ILL'
    config['default.instrument'] = 'D17'
    config.appendDataSearchSubDir('ILL/D17/')

.. testcode:: MultipleAngles

    ws = ReflectometryILLAutoProcess(
     Run='541853,541854',
     DirectRun='541838,541839',
     WavelengthLowerBound=[3.5,3.5],
     WavelengthUpperBound=[25.,22.],
     GlobalScaleFactor=0.13,
     DeltaQFractionBinning=0.5
    )
    print('The R(Q) workspace at first angle has {0} points'.format(ws.getItem(0).blocksize()))
    print('The R(Q) workspace at second angle has {0} points'.format(ws.getItem(1).blocksize()))
    print('The R(Q) workspace at second angle has {0} points'.format(ws.getItem(2).blocksize()))

.. testoutput:: MultipleAngles

    The R(Q) workspace at first angle has 186 points
    The R(Q) workspace at second angle has 94 points
    The R(Q) workspace at second angle has 280 points


.. testcleanup:: MultipleAngles

    import os
    os.remove("ws_0.out")
    os.remove("ws_1.out")

**Example - Full treatment with 3 angles and multiple numors summed**

.. code-block:: python

   from mantid.simpleapi import *
   config['default.facility'] = 'ILL'
   config['default.instrument'] = 'D17'
   config.appendDataSearchSubDir('ILL/D17/')
   name = 'Thick_HR_5'
   directBeams = '397812,397806,397808'
   reflectedBeams = '397826+397827,397828,397829+397830+397831+397832'
   foregroundWidth = [4,5,8]
   wavelengthLower = [3., 1.6, 2.]
   wavelengthUpper = [27., 25., 25.]
   angleOffset = 2
   angleWidth = 10
   ReflectometryILLAutoProcess(
       Run=reflectedBeams,
       DirectRun=directBeams,
       OutputWorkspace=name,
       SummationType='Incoherent',
       AngleOption='SampleAngle',
       DirectLowAngleFrgHalfWidth=foregroundWidth,
       DirectHighAngleFrgHalfWidth=foregroundWidth,
       DirectLowAngleBkgOffset=angleOffset,
       DirectLowAngleBkgWidth=angleWidth,
       DirectHighAngleBkgOffset=angleOffset,
       DirectHighAngleBkgWidth=angleWidth,
       ReflLowAngleFrgHalfWidth=foregroundWidth,
       ReflHighAngleFrgHalfWidth=foregroundWidth,
       ReflLowAngleBkgOffset=angleOffset,
       ReflLowAngleBkgWidth=angleWidth,
       ReflHighAngleBkgOffset=angleOffset,
       ReflHighAngleBkgWidth=angleWidth,
       WavelengthLowerBound=wavelengthLower,
       WavelengthUpperBound=wavelengthUpper,
       DeltaQFractionBinning=0.5
   )
   import matplotlib.pyplot as plt
   from mantid import plots
   plt.style.use('classic')
   plt.rcParams['figure.figsize'] = (12, 7)
   plt.rcParams['font.size'] = 18
   wsMantid = mtd['Thick_HR_5_stitched']
   fig, ax = plt.subplots(subplot_kw={'projection':'mantid'})
   ax.plot(wsMantid, 'b', label="RoundRobin")
   ax.errorbar(wsMantid,'rs', 'b', markersize=0.1, label=None)
   ax.set_xlim(0.01, 0.2)
   ax.set_ylim(0.000001, 2.)
   ax.set_yscale("log")
   ax.set_xscale("log")
   ax.set_xlabel(r'Q [$\AA^{-1}$]')
   ax.set_ylabel('R')
   ax.legend()
   fig.show()

References
----------

.. [#Gutfreund] P. Gutfreund, T. Saerbeck, M. A. Gonzalez, E. Pellegrini, M. Laver, C. Dewhurst, R. Cubitt,
            `Towards generalized data reduction on a chopper-based time-of-flight neutron reflectometer.`
            `J. Appl. Cryst. (2018). 51, 606-615, <https://doi.org/10.1107/S160057671800448X>`_

.. categories::

.. sourcelink::
