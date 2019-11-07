.. algorithm::

.. summary::

Description
-----------

This algorithm executes the full data reduction for ILL reflectometers D17 and FIGARO in TOF mode (specular reflection).
It supports the treatment of measurements at several angles together.
It offers incoherent (sum along constant :math:`\lambda`) or coherent (sum along constant :math:`Q_{z}`) methods of peak summation.
Treatment of polarized measurements is also provided.

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

Workflow
--------

The processing of the direct and reflected beams are shown in the following diagrams.

.. diagram:: ReflectometryILLAutoProcess1-v1_wkflw.dot

.. diagram:: ReflectometryILLAutoProcess2-v1_wkflw.dot

Usage
-----

.. include:: ../usagedata-note.txt

**Example - ReflectometryILLAutoProcess1**

.. testcode:: ReflectometryILLAutoProcess1

   angleOffset = 10
   angleWidth = 20
   foreground = [5]

   ws = ReflectometryILLAutoProcess(
       Run='ILL/D17/317370.nxs',
       DirectRun='ILL/D17/317369.nxs',
       DirectLowAngleFrgHalfWidth=foreground,
       DirectHighAngleFrgHalfWidth=foreground,
       DirectLowAngleBkgOffset=angleOffset,
       DirectLowAngleBkgWidth=angleWidth,
       DirectHighAngleBkgOffset=angleOffset,
       DirectHighAngleBkgWidth=angleWidth,
       ReflLowAngleFrgHalfWidth=foreground,
       ReflHighAngleFrgHalfWidth=foreground,
       ReflLowAngleBkgOffset=angleOffset,
       ReflLowAngleBkgWidth=angleWidth,
       ReflHighAngleBkgOffset=angleOffset,
       ReflHighAngleBkgWidth=angleWidth,
       WavelengthLowerBound=3.5,
       WavelengthUpperBound=25.
    )

    print('The R(Q) workspace has {0} points, from {1} to {2} inverse Angstrom'.format(ws.getItem(0).blocksize(), ws.getItem(0).readX(0)[0], ws.getItem(0).readX(0)[-1]))

.. testoutput:: ReflectometryILLAutoProcess1

    The R(Q) workspace has 688 points, from 0.00678002009846 to 0.0479086988309 inverse Angstrom

.. relatedalgorithms::

.. properties::

.. categories::

.. sourcelink::
