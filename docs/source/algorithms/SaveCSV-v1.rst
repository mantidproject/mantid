.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

The format of the saved ascii CSV file for a 1D worksspace consists of
three columns where the numbers of each row are seperated by the
Separator and each line by the LineSeparator.

The format of the saved CSV file for a 2D workspace is as follows::

    A      0, 200, 400, 600, ..., 50000
    0     10,   4, 234,  35, ...,    32
    1      4, 234,   4,   9, ...,    12
    A      0, 100, 200, 300, ..., 25000
    2     34,   0,   0,   0, ...,    23

   ERRORS
    0    0.1, 3.4, 2.4, 3.5, ...,     2
    1    3.1, 3.3, 2.5, 3.5, ...,     2
    2    1.1, 3.3, 2.4,   5, ...,   2.4

where for the matrix above the ERRORS line the first column shows the
content of the numbers on the of the same line; i.e. 'A' is followed by
x-axis values (e.g. TOF values) and any number (e.g. '2') followed by
y-axis values (e.g. neutron counts). Multiple 'A' may be present to
allow for the a-axis to change. So in the example above the saved 2D
workspace consists of three histograms (y-axes) where the first two have
the same x-axis but the third histogram has a different x-axis.

The matrix following the ERRORS line lists the errors as recorded for
each histogram.

Usage
-----

.. testcode::

  import os
  # Create a file path in the user home directory
  filePath = os.path.expanduser('~/SavedCSVFile.csv')

  # Create a workspace
  ws=CreateSampleWorkspace()
  # Save it in Nexus format
  SaveCSV(ws,filePath)

.. testcleanup::

  os.remove(filePath)

.. categories::

.. sourcelink::
