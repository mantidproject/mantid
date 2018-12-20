.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithms writes a cal file with the selection column set to the
masking status of the workspaces provided. The offsets and grouping
details of the cal file are not completed, so you would normally use
:ref:`MergeCalFiles <algm-MergeCalFiles>` afterwards to import these values from another file.

Usage
-----

.. testcode::

  import os

  # Create a workspace containing some data.
  ws = CreateSampleWorkspace()

  # Mask two detectors by specifying detector IDs 101 and 103
  MaskDetectors(ws,DetectorList=[101,103])

  # Create a file path in the user home directory
  calFilePath = os.path.expanduser('~/MantidUsageExample_CalFile.cal')

  # Save the masking in a cal file.
  MaskWorkspaceToCalFile( ws, calFilePath );

  # Read the saved file back
  f = open( calFilePath, 'r' )
  calFile = f.read().split('\n')
  f.close()

  # Print out first 10 lines of the file
  for line in calFile[:10]:
    print(line)

Output
######

.. testoutput::

  # basic_rect detector file
  # Format: number      UDET       offset       select    group
          0             100       0.0000000        1        1
          1             101       0.0000000        0        0
          2             102       0.0000000        1        1
          3             103       0.0000000        0        0
          4             104       0.0000000        1        1
          5             105       0.0000000        1        1
          6             106       0.0000000        1        1
          7             107       0.0000000        1        1

.. testcleanup::

  os.remove( calFilePath )

.. categories::

.. sourcelink::
