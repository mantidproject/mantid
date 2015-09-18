.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm outputs the data in ASCII, it takes a focused workspaces
which would generate an OpenGenie ascii file with data saved inside.
The algorithms will write out for every spectra, the x, y and e axis.
It also imports all the logs available in the workspace and the variables
are then written sorted by alphabetical order which matches the format of
OpenGenie ascii file.

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
