.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm requires a workspace that is both in d-spacing, but has
also been preprocessed by the :ref:`algm-CrossCorrelate`
algorithm. In this first step you select one spectrum to be the
reference spectrum and all of the other spectrum are cross correlated
against it. Each output spectrum then contains a peak whose location
defines the offset from the reference spectrum.

The algorithm iterates over each spectrum in the workspace and fits a PeakFunction (default is a
:ref:`Gaussian <func-Gaussian>` function) to the reference peaks. The fit is used
to calculate the centre of the fitted peak, and the offset is then
calculated as:

:math:`-peakCentre*step/(dreference+PeakCentre*step)`

This is then written into a `.cal file <http://www.mantidproject.org/CalFile>`__ for every detector
that contributes to that spectrum. All of the entries in the cal file
are initially set to both be included, but also to all group into a
single group on :ref:`algm-DiffractionFocussing`. The
:ref:`algm-CreateCalFileByNames` algorithm can be used to
alter the grouping in the cal file.

Usage
-----

.. testcode::

  import os

  # Create a workspace with a Gaussian peak in the centre.
  ws = CreateSampleWorkspace(Function='User Defined',UserDefinedFunction='name=Gaussian,Height=1,PeakCentre=10,Sigma=1',XMin=0,XMax=20,BinWidth=0.1)
  ws.getAxis(0).setUnit( 'dSpacing' )

  # Generate a file path to save the .cal file at.
  calFilePath = os.path.expanduser( '~/MantidUsageExample_CalFile.cal' )

  # Run the algorithm
  msk = GetDetectorOffsets(ws,0.001,10.0,0, 10, calFilePath)

  # Read the saved .cal file back in
  f = open( calFilePath, 'r' )
  file = f.read().split('\n')
  f.close()

  # Print out first 10 lines of the file
  print("{} ...".format(file[0][:55]))
  for line in file[1:10]:
      print(line)

Output
######

.. testoutput::

  # Calibration file for instrument basic_rect written on ...
  # Format: number    UDET         offset    select    group
          0            100     -0.0014265       1       1
          1            101     -0.0014265       1       1
          2            102     -0.0014265       1       1
          3            103     -0.0014265       1       1
          4            104     -0.0014265       1       1
          5            105     -0.0014265       1       1
          6            106     -0.0014265       1       1
          7            107     -0.0014265       1       1

.. testcleanup::

  os.remove( calFilePath )

.. categories::

.. sourcelink::
