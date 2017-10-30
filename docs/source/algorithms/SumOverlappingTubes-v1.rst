.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm takes an input workspace, with a PSD tube base instrument, and converts the workspace to a one or two-dimensional workspace of counts as a function of height and scattering angle. There are options to either see the data as it is, or apply a correction to straighten the Debye-Scherrer cones.

This algorithm is written for the geometry of D2B, where the detector tubes have the same radial distance from the sample, and rotate around the sample in the horizontal plane during a detector scan. This algorithm could be generalised to support other instrument geometries.

General Behavior
#################

For the output workspace a grid is made, the x-axis is the scattering angle for the tube center and the y-axis is the tube height. Counts from the input workspaces are then put onto this grid. Where one of the entries in the scattering angle is between two angles the count are split between the two, unless the ScatteringAngleTolerance option is set. 

If the Normalise option is set then the counts for each point in the OutputWorkspace are divided by the number of contributing points to that pixel. The scaling goes as

.. math:: C_{scaled, i} = \frac{N_{max}}{N_{i}} C_i

where :math:`C_{scaled, i}` is the scaled counts, :math:`C_i` the raw counts, :math:`N_{max}` is the maximum number of detectors contributing to any point in the OutputWorkspace, and :math:`N_{i}` is the number of detectors contributing to the point being scaled.

2D Option
#########

In this case the x-axis, the scattering angle, is the scattering angle of the tube centre. The height is the pixel height in the tube.

2DStraight Option
#################

In this case the x-axis is the true scattering angle of a pixel, so the Debye-Scherrer cones are effectively straightened. The x-axis is strictly positive in this case, the negative angles from the 2D case are put in the same bins as the equivalent positive angles. The height is the pixel height in the tube as for the 2D Option.

1DStraight Option
#################

This is effectively the 2DStraight option, with a single bin in the y-axis. For this case only the HeightAxis option can be given as a minimum and maximum.

Usage
-----
**Example - a simple example of running SumOverlappingTubes.**

.. testcode:: SumOverlappingTubes2DComponent

    ws = Load(...)
    ws = SumOverlappingTubes(OutputType='2D', ScatteringAngleBinning='0.05', ComponentForHeightAxis='tube_1')
    print("Workspace has %d spectra" % ws.getNumberHistograms())

Output:

.. testoutput:: SumOverlappingTubes2DComponent

    ...

.. categories::

.. sourcelink::
