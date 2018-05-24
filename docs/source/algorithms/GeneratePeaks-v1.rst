.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Generate a workspace by summing over the peak functions and optionally background functions.
The peaks' and background'
parameters are either (1) given in a `TableWorkspace <http://www.mantidproject.org/TableWorkspace>`_
or (2) given by an array of doubles.

Function Parameters
###################

There are 2 different approaches to input parameter values of peak and background function.

TableWorkspace
==============

Peak and background parameters must have the following columns, which are case
sensitive in input `TableWorkspace <http://www.mantidproject.org/TableWorkspace>`_

The definition of this table workspace is consistent with the output
peak and background parameter `TableWorkspace <http://www.mantidproject.org/TableWorkspace>`_
of algorithm FindPeaks.

The following table contains the effective peak and background parameters.


+------+--------------------+-------+
|Column|          Name      |Comment|
+======+====================+=======+
|  1   | spectrum           |       |
+------+--------------------+-------+
|  2   | centre             |       |
+------+--------------------+-------+
|  3   | height             |       |
+------+--------------------+-------+
|  4   | width              | FWHM  |
+------+--------------------+-------+
|  5   | backgroundintercept|  A0   |
+------+--------------------+-------+
|  6   | backgroundslope    |  A1   |
+------+--------------------+-------+
|  7   | A2                 |  A2   |
+------+--------------------+-------+
|  8   | chi2               |       |
+------+--------------------+-------+

Double Array
============

An alternative way to input function parameters is to specify parameter values
in property 'PeakParameterValues' and 'BackgroundParameterValues'.

In this case, there is only one peak function that can be specified and generated.

The order of the parameters are pre-determined,
which can be found in the dropdown list of 'PeakType' and 'BackgroundType'.
For example in 'PeakType', there is an item named 'Gaussian(Height,PeakCentre, Sigma)'.
Therefore, the order of parameters for Gaussian is height, peak centre and sigma.
It does not matter whether 'Gaussian' or 'Gaussian(Height,PeakCentre, Sigma)' is selected.


Effective Peak and Background Parameters
########################################

GeneratePeak supports effective peak and background parameters.

For peak parameters, the effective parameters are centre, height and FWHM.
This order must be followed if the parameter values are input through 'PeakParameterValues'.

For background parameters, the effective parameters are
interception, slope and A2.
This order must be followed if the parameter values are input through 'BackgroundParameterValues'.


Output
######

Output can be either pure peak with 'GenerateBackground' deselected or peak and background.

If 'InputWorkspace', which is optional, is not given, then a single spectrum workspace can be generatged from scratch
according to property 'BinningParameters'.


Usage
-----

Generate peaks from a TableWorkspace
####################################

.. testcode:: GeneratePeakFromTable

  # Create an input Tableworkspace by FindPeaks
  Load(Filename='focussed.nxs', OutputWorkspace='focussed', LoaderName='LoadNexusProcessed',
    LoaderVersion=1, LoadHistory=False)
  FindPeaks(InputWorkspace='focussed', WorkspaceIndex=3, BackgroundType='Quadratic', PeaksList='PeakList3')

  # Geneate peaks
  GeneratePeaks(PeakParametersWorkspace='PeakList3', PeakType='Gaussian', BackgroundType='Quadratic',
        InputWorkspace='focussed', NumberWidths=5, OutputWorkspace='GaussianPeak', IsRawParameter=False)

  # Print
  outws = mtd["GaussianPeak"]
  vecx3 = outws.readX(3)
  vecy3 = outws.readY(3)
  for i in range(4277, 4283):
    print("X = {:.6f}, Y = {:.6f}".format(vecx3[i], vecy3[i]))

.. testcleanup:: GeneratePeakFromTable

  DeleteWorkspace(Workspace=outws)
  DeleteWorkspace(Workspace="focussed")
  DeleteWorkspace(Workspace="PeakList3")

Output:

.. testoutput:: GeneratePeakFromTable

  X = 2.137026, Y = 24074.469544
  X = 2.139414, Y = 25091.516123
  X = 2.141805, Y = 25481.643702
  X = 2.144199, Y = 25120.243602
  X = 2.146595, Y = 24105.695010
  X = 2.148994, Y = 22713.122811


Generate peaks from arrays
##########################


.. testcode:: GeneratePeakFromArray

  GeneratePeaks(PeakType='Gaussian (Height, PeakCentre, Sigma)', PeakParameterValues='10,1,0.2',
      BackgroundType='Linear (A0, A1)', BackgroundParameterValues='5,1',
      BinningParameters='0,0.01,20', NumberWidths=5, OutputWorkspace='GaussianPeak')

  outws = mtd["GaussianPeak"]
  for i in [92,93,94,95]:
      print("X = {:.6f}, Y = {:.6f}".format(outws.readX(0)[i], outws.readY(0)[i]))


.. testcleanup:: GeneratePeakFromArray

  DeleteWorkspace(Workspace=outws)

Output:

.. testoutput:: GeneratePeakFromArray

  X = 0.920000, Y = 15.151163
  X = 0.930000, Y = 15.335881
  X = 0.940000, Y = 15.499975
  X = 0.950000, Y = 15.642332

.. categories::

.. sourcelink::
