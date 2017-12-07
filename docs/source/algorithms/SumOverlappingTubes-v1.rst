.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm takes an input workspace, a group workspace or a selection of workspaces, with a PSD tube instrument, and converts the workspace to a one or two-dimensional workspace of counts as a function of height and scattering angle. There are options to either see the data as it is, or apply a correction to straighten the Debye-Scherrer cones.

This algorithm is written for the geometry of D2B, where the detector tubes have the same radial distance from the sample, and rotate around the sample in the horizontal plane during a detector scan. This algorithm could be generalised to support other instrument geometries in the future.

General Behavior
#################

For the output workspace a grid is made, the x-axis is the scattering angle and the y-axis is vertical distance from the centre of the tube (referred to as the tube height). Counts from the input workspaces are then put onto this grid. Where one of the entries in the scattering angle is between two angles the count are split between the two, unless the ScatteringAngleTolerance option is set.

If the Normalise option is set then the counts for each point in the OutputWorkspace are divided by the number of contributing points to that pixel. The scaling goes as

.. math:: C_{scaled, i} = \frac{N_{max}}{N_{i}} C_i

where :math:`C_{scaled, i}` is the scaled counts, :math:`C_i` the raw counts, :math:`N_{max}` is the maximum number of tube pixels contributing to any point in the OutputWorkspace, and :math:`N_{i}` is the number of tube pixels contributing to the point being scaled.

2D Option
+++++++++

In this case the x-axis, the scattering angle, is the scattering angle of the tube centre. The height is the pixel height in the tube.

2DStraight Option
+++++++++++++++++

In this case the x-axis is the true scattering angle of a pixel, so the Debye-Scherrer cones are effectively straightened. The x-axis is strictly positive in this case, the negative angles from the 2D case are put in the same bins as the equivalent positive angles. The height is the pixel height in the tube as for the 2D Option.

1DStraight Option
+++++++++++++++++

This is effectively the 2DStraight option, with a single bin in the y-axis. For this case only the HeightAxis option can be given as a minimum and maximum.

Instrument Parameters
+++++++++++++++++++++

If the instrument parameter ``mirror_detector_angles`` is set to true, then output scattering angles are effectively multiplied by :math:`-1`. This is used for D2B where the instrument is mirrored compared with other Powder Diffraction instruments at the ILL.

Usage
-----
**Example - an example of running SumOverlappingTubes in the 2D case.**

.. testcode:: SumOverlappingTubes2DComponent

    ws_508093 = Load('ILL/D2B/508093.nxs')
    ws = SumOverlappingTubes(InputWorkspaces=ws_508093, OutputType='2D', ComponentForHeightAxis='tube_1')
    print('X Size: ' + str(ws.blocksize()) + ', Y Size: ' + str(ws.getNumberHistograms()))
    print('Counts: ' + str(ws.dataY(63)[2068:2078]))
    print('Errors: ' + str(ws.dataE(63)[2068:2078]))

Output:

.. testoutput:: SumOverlappingTubes2DComponent

    X Size: 3200, Y Size: 128
    Counts: [  4.46083333   9.72705882  13.1016      19.74509804  27.49234043
      24.62941176  30.25076923  23.2776      10.01591837   1.06      ]
    Errors: [ 2.03598273  3.00072517  3.46216187  4.1661015   5.29106717  4.88935243
      5.28366714  4.68457643  3.1474581   0.96348311]

**Example - an example of running SumOverlappingTubes in the 1DStraight case.**

.. testcode:: SumOverlappingTubes1DHeightRange

    ws_508093 = Load('ILL/D2B/508093.nxs')
    ws = SumOverlappingTubes(InputWorkspaces=ws_508093, OutputType='1DStraight', CropNegativeScatteringAngles=True, HeightAxis='-0.05,0.05')
    print('X Size: ' + str(ws.blocksize()) + ', Y Size: ' + str(ws.getNumberHistograms()))
    print('Counts: ' + str(ws.dataY(0)[2068:2078]))
    print('Errors: ' + str(ws.dataE(0)[2068:2078]))

Output:

.. testoutput:: SumOverlappingTubes1DHeightRange

    X Size: 2975, Y Size: 1
    Counts: [ 127.08681254  131.10979889  201.71370827  233.54556754  296.48915172
      286.24790285  260.59967375  188.05934431  143.70447835  113.86610964]
    Errors: [ 12.79221591  12.49380558  15.76125177  16.4410194   20.01917432
      19.39744376  18.06430971  15.28768958  13.52007099  11.44274953]

.. categories::

.. sourcelink::
