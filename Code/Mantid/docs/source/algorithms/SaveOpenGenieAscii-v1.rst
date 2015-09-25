.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm outputs the data in ASCII file, it takes a focused
workspaces which would generate an OpenGenie ASCII file with data
saved inside. The algorithm will assume focused data (which
contains single spectrum) has been provided, but if a workspace
with multiple spectra is passed, the algorithm will save the first
spectrum, while ignoring the rest. The algorithms will write out
the x, y and e axis for each spectra. The x, y, e axis are
sorted by alphabetical order in the file, which matches the format
of OpenGenie ASCII file.

Usage
-----
**Example - a basic example using SaveOpenGenieAscii.**

.. testcode:: ExSaveOpenGenie

    import os

    ws = CreateSampleWorkspace()
    ws = ExtractSingleSpectrum(ws, WorkspaceIndex=0)
    file_name = "myworkspace.ascii"
    path = os.path.join(os.path.expanduser("~"), file_name)

    SaveOpenGenieAscii(ws, path)

    path = os.path.join(os.path.expanduser("~"), "myworkspace.ascii")
    print os.path.isfile(path)


Output:

.. testoutput:: ExSaveOpenGenie

    True

.. testcleanup:: ExSaveOpenGenie

    import os
    def removeFiles(files):
      for ws in files:
        try:
          path = os.path.join(os.path.expanduser("~"), ws)
          os.remove(path)
        except:
          pass

    removeFiles(["myworkspace.ascii"])

.. categories::

.. sourcelink::
