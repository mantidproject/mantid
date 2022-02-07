
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm performs stitching of multiple 2D workspaces. Stitching is the operation of scaling one workspace to match the scale of another one in the overlap region and the combination of the workspaces after scale matching.
For each pair of adjacent workspaces in the input, it will find the overlap, interpolate one to another in the overlap region, make the ratio and find the scaling factor.
Then each workspace will be scaled with that factor, and finally all the workspaces will be combined.

Inputs
------

InputWorkspaces
###############

The list of input workspace or workspace group names. The workspace groups will be flattened to a list of workspaces.
Note that because of this type of input that relies on WorkspaceGroups, this algorithm is currently supported only for named workspaces present on ADS.
The inputs can be in any order in terms of their x-axis extent. The algorithm will first sort them ascending in terms of x-intervals.
Then it will start stitching iteratively to the left and to the right starting from the reference workspace. The input workspaces must be point-data and must have common bins.

ReferenceWorkspace
##################

The name of the workspace that will not be scaled. This means that other workspaces will be scaled to match up the scale of the reference one.
The reference is optional; if it is not specified, the workspace with the lowest x-axis interval will be taken as reference, which means that the stitching will iterate left-to-right.

CombinationBehaviour
####################

At the moment the only option is interleave, which means that all the points from all the inputs will be preserved and the output will be sorted by the x-axis values.

ScaleFactorCalculation
######################

By default it will calculate the median of the point-wise ratios. This is robust in the sense that it is not sensitive to outliers.
This can be set to manual, in which case the scale factors can be specified by the user.

ManualScaleFactors
##################

Those must be in the same order as the workspaces in the original input list. By setting those to 1. one could enforce no-scale stitching.

TieScaleFactors
###############

If this is checked, one will enforce that the scale factor calculated between two workspaces is the global median of point-wise ratios in the overlapping region and is hence constant for all the spectra.
Otherwise, for each spectra, it will find the median for that spectrum and scale each spectrum with its own factor.
Manual factors can only be considered as tied, i.e. they will be applied to all the spectra.

OutputWorkspace
###############

The workspace containing the stitched outputs.

OutputScaleFactorsWorkspace
###########################

An optional output workspace to store the scale factors.
If manual or tied scale factors are used, this will be a single spectrum workspace containing those factors in the original order.
Without ties this will contain as many spectra as the inputs, each of which will contain the scale factors for that spectrum.

Usage
-----

**Example - Stitch**

.. testcode:: StitchExample

  np.random.seed(179864)
  x1 = np.linspace(0,100,11)
  y1 = 5 + np.sin(x1) + np.random.normal(scale=0.001)
  ws1 = CreateWorkspace(DataX=x1,DataY=y1,NSpec=1)
  x2 = np.linspace(50,150,11)
  y2 = 7 + np.cos(x2) + np.random.normal(scale=0.001)
  ws2 = CreateWorkspace(DataX=x2,DataY=y2,NSpec=1)
  st = Stitch([ws1,ws2])
  print(f'Stitched curve has {st.blocksize()} points')

Output:

.. testoutput:: StitchExample

  Stitched curve has 22 points

.. categories::

.. sourcelink::
