.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm takes an input workspace, a group workspace or a selection of workspaces, with a PSD tube instrument, and converts the workspace to a one or two-dimensional workspace of counts as a function of height and scattering angle. There are options to either see the data as it is, or apply a correction to straighten the Debye-Scherrer cones.

This algorithm is written for the geometry of D2B, where the detector tubes have the same radial distance from the sample, and rotate around the sample in the horizontal plane during a detector scan. This algorithm could be generalised to support other instrument geometries in the future.

General Behavior
#################

For the output workspace a grid is made, the x-axis is the scattering angle and the y-axis is vertical distance from the centre of the tube (referred to as the tube height). Counts from the input workspaces are then put onto this grid. Where one of the entries in the scattering angle is between two angles the count are split between the two, unless the ScatteringAngleTolerance option is set.

If the Normalise option is set then the counts for each point in the OutputWorkspace are divided by the number of contributing points to that pixel. The scaling goes as

.. math:: C_{scaled, i} = \frac{C_i}{N_{i}}

where :math:`C_{scaled, i}` is the scaled counts, :math:`C_i` the raw counts, and :math:`N_{i}` is the number of tube pixels contributing to the point being scaled.

2DTubes Option
++++++++++++++

In this case the x-axis, the scattering angle, is the scattering angle of the tube centre. The height is the pixel height in the tube.

2D Option
+++++++++

In this case the x-axis is the true scattering angle of a pixel, so the Debye-Scherrer cones are effectively straightened. The x-axis is strictly positive in this case, the negative angles from the 2D case are put in the same bins as the equivalent positive angles. The height is the pixel height in the tube as for the 2D Option.

1D Option
+++++++++

This is the same as the 2D option, with a single bin in the y-axis.

Instrument Parameters
+++++++++++++++++++++

If the instrument parameter ``mirror_detector_angles`` is set to true, then output scattering angles are effectively multiplied by :math:`-1`. This is used for D2B where the instrument is mirrored compared with other Powder Diffraction instruments at the ILL.

SplitCounts
+++++++++++

The counts are split between bins, if the difference between to angles is larger than the scattering angle step times the ``ScatteringAngleTolerance``.
By default there is no splitting.

CropNegativeScatteringAngles
++++++++++++++++++++++++++++

When cropping is requested the bins that are fully on the negative side, will be dropped. However if there is a bin with one edge negative, the other positive, this bin will be preserved.

ScatteringAngleBinning
++++++++++++++++++++++

This can be a comma separated list of start, step, end. Alternatively, if single number is given, it will be taken as the step.
The start and end will be computed from the input workspaces.

OutputWorkspace
+++++++++++++++

Output is a histogram workspace containing the summed/averaged counts.

Usage
-----
**Example - an example of running SumOverlappingTubes in the 2DTubes case.**

.. testcode:: SumOverlappingTubes2DComponent

    ws_508093 = Load('ILL/D2B/508093.nxs')
    ws = SumOverlappingTubes(InputWorkspaces=ws_508093, OutputType='2DTubes', MirrorScatteringAngles=True)
    print('X Size: ' + str(ws.blocksize()) + ', Y Size: ' + str(ws.getNumberHistograms()))

Output:

.. testoutput:: SumOverlappingTubes2DComponent

    X Size: 3200, Y Size: 128

**Example - an example of running SumOverlappingTubes in the 1D case.**

.. testcode:: SumOverlappingTubes1DHeightRange

    ws_508093 = Load('ILL/D2B/508093.nxs')
    ws = SumOverlappingTubes(InputWorkspaces=ws_508093, OutputType='1D', CropNegativeScatteringAngles=True, HeightAxis='-0.05,0.05', MirrorScatteringAngles=True)
    print('X Size: ' + str(ws.blocksize()) + ', Y Size: ' + str(ws.getNumberHistograms()))

Output:

.. testoutput:: SumOverlappingTubes1DHeightRange

    X Size: 2975, Y Size: 1

.. categories::

.. sourcelink::
