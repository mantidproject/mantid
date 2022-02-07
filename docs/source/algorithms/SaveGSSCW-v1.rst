.. algorithm::

.. summary::

.. relatedalgorithms::

SaveGSS-v1

.. properties::

Description
-----------

Saves a constant wavelength diffraction pattern in units Degrees to
FXYE GSAS format.

From the GSAS manual a description of the format options:
If TYPE is 'FXY' or 'FXYE' then the data records give the position,
intensity and in the case of 'FXYE' the esd in the intensity with one set per record and in free format.
The values may also be in scientific form. The position is either in centidegrees for CW data
or microseconds for TOF data. Each record must be padded out to 80 characters and end with CR/LF.


Usage
-----
**Example - a basic example using SaveGSSCW.**

.. testcode:: ExSaveGSSCWHB2A

    import os
    # Create a workspace to save
    ws = CreateSampleWorkspace(OutputWorkspace="SaveGSSWorkspace", XUnit='Degrees')
    ws = ExtractSingleSpectrum(ws, WorkspaceIndex=0)

    # Save to the users home directory
    file_name = "myworkspace.ascii"
    path = os.path.join(os.path.expanduser("~"), file_name)
    SaveGSS(ws, path)

    # Does the file exist
    path = os.path.join(os.path.expanduser("~"), file_name)
    print(os.path.isfile(path))

Output:

.. testoutput:: ExSaveGSSCWHB2A

    True

.. testcleanup:: ExSaveGSSCWHB2A

    import os

    def removeFiles(files):
      for ws in files:
        try:
          path = os.path.join(os.path.expanduser("~"), ws)
          os.remove(path)
        except:
          pass

    removeFiles([file_name])
    DeleteWorkspace("SaveGSSWorkspace")


.. categories::

.. sourcelink::
