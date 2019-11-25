.. algorithm::

.. summary::

Description
-----------

This algorithm executes the full data reduction for ILL reflectometers D17 and FIGARO in TOF mode (specular reflection).
It supports the treatment of measurements at several angles together.
It offers incoherent (sum along constant :math:`\lambda`) or coherent (sum along constant :math:`Q_{z}`) methods of peak summation.
For the incoherent method, the reflectivity curve is calculated by dividing the already summed foreground of the reflected beam to the summed foreground of the direct beam.
For the coherent method, first the reflected beam data is divided by the direct beam data in 2D, then the ratio is summed along the lines of the constant :math:`Q_{z}`.
Treatment of polarized measurements (with or without analysers) is also provided.

Input
-----

The mandatory inputs are comma separated list of nexus files for direct and reflected beam measurements.
`,` stands as the separator of different angle configurations.
`+` (sum) or `-` (range sum) operations can be used to sum different files at the same instrument configuration.
When summing, the metadata (e.g. acquisition time) will also be summed, so that the subsequent normalisation is handled correctly.
There must be the same number of angle configurations both for direct and reflected beam inputs.

Output
------

The output is a workspace group that contains the calculated reflectivity curves as a function of the momentum transfer.
The output is point data and has the calculated Q resolution attributed to it.
There is a separate output for each angle configuration.
An automatically stitched result is also produced.
Stitch in this case just takes the union of all the initial points without merging or removal, only scaling can be applied.
The outputs can be readily saved by :ref:`SaveReflectometryAscii <algm-SaveReflectometryAscii>` algorithm for further analysis.

BraggAngle
----------

If user specified :math:`\theta` angles are provided, they will be used.
Otherwise `SampleAngle` or `DetectorAngle` (default) option is executed.

Options
-------

Many options can be specified as a single value, which will be applied to all the angle configurations, or as a list of values.
In the case of the latter, the list must be of the same size, as many different angle configurations there are.

Direct Beam Caching
-------------------

The processed direct beam run will be cached in Analysis Data Service in order to save significant time when multiple samples correspond to the same reflected beam.
The direct beam run number is put in the name of the workspace. However, if the same direct beam must be used with different options (e.g. different wavelength ranges), the caching will result in an error in subsequent reductions due to incompatibility.
In such circumstances it must be disabled.

Workflow
--------

The processing of the direct and reflected beams are shown in the following diagrams.

.. diagram:: ReflectometryILLAutoProcess1-v1_wkflw.dot

.. diagram:: ReflectometryILLAutoProcess2-v1_wkflw.dot

Usage
-----

.. include:: ../usagedata-note.txt

**Example - Single Angle**

.. testcode:: SingleAngle

    ws = ReflectometryILLAutoProcess(
     Run='317370',
     DirectRun='317369',
     WavelengthLowerBound=3.5,
     WavelengthUpperBound=24.5
    )
    print('The R(Q) workspace has {0} points, from {1} to {2} inverse Angstrom'.format(ws.getItem(0).blocksize(), ws.getItem(0).readX(0)[0], ws.getItem(0).readX(0)[-1]))

.. testoutput:: SingleAngle

    The R(Q) workspace has 672 points, from 0.00718855456602 to 0.0497798802388 inverse Angstrom

.. relatedalgorithms::

.. properties::

.. categories::

.. sourcelink::
