.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Save a :ref:`PeaksWorkspace <PeaksWorkspace>` to a ISAW-style ASCII .peaks file.

The :ref:`PeaksWorkspace <PeaksWorkspace>` consists of individual Peaks, each of which has an individual PeakShape relating to the way the workspace has been integrated. The PeakShape information is not saved out in the ISAW peaks format. To save the full peak information, use the NeXus format :ref:`SaveNexus <algm-SaveNexus>`.

Usage
-----

.. testcode::

  import os

  # Prepare a PeaksWorkspace to save.

  # Create a run workspace
  ws = CreateSampleWorkspace()
  # Create a peaks workspace
  pws = CreatePeaksWorkspace(ws)

  # Add two peaks to the peaks workspace
  AddPeak( pws, ws, TOF=100, DetectorID=101, Height=1 )
  AddPeak( pws, ws, TOF=200, DetectorID=102, Height=2 )

  # Save the peaks workspace to a file in the user's home directory
  isawPeaksFilePath = os.path.expanduser('~/MantidUsageExample_ISawFile.peaks')
  SaveIsawPeaks( pws, isawPeaksFilePath )

  # Read the saved file back in
  f = open( isawPeaksFilePath, 'r' )
  file = f.read().split('\n')
  f.close()

  # Print out 11 first lines of the peaks file, skipping line #8 because it's
  # different on different systems and breaks tests
  for line in file[:8]:
      # print the line stripping any ending white spaces
      print line.rstrip()
  for line in file[9:11]:
      # print the line stripping any ending white spaces
      print line.rstrip()

Output
######

.. testoutput::
   :options: +ELLIPSIS

   Version: 2.0  Facility: SNS  Instrument: basic_rect  Date: 1990-01-01T00:00:01
   6         L1    T0_SHIFT
   7  1000.0000       0.000
   4 DETNUM  NROWS  NCOLS   WIDTH   HEIGHT   DEPTH   DETD   CenterX   CenterY   CenterZ    BaseX    BaseY    BaseZ      UpX      UpY      UpZ
   5      1     10     10  8.0000  8.0000   0.2000 500.00    0.0000    0.0000  500.0000  1.00000  0.00000  0.00000  0.00000  1.00000  0.00000
   0  NRUN DETNUM     CHI      PHI    OMEGA       MONCNT
   1     0      1    0.00     0.00     0.00            0
   2   SEQN    H    K    L     COL      ROW     CHAN        L2   2_THETA        AZ         WL         D      IPK       INTI    SIGI  RFLG
   3      2    0    0    0    0.00     1.00      100   500.001   0.00160   1.57080   0.026374   16.4835        0       1.00    1.00   310
   3      3    0    0    0    0.00     2.00      200   500.003   0.00320   1.57080   0.052747   16.4835        0       2.00    1.41   310

.. testcleanup::

  os.remove( isawPeaksFilePath )

.. categories::
