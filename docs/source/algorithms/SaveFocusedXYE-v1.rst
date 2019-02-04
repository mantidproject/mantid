.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm outputs the data in ASCII as a 3 column X, Y ,E format
for use in subsequent analysis by other programs. The output files can
be read for example into FullProf with format instrument=10.

For data where the focusing routine has generated several spectra (for
example, multi-bank instruments), the option is provided for saving all
spectra into a single file, separated by headers, or into several files
that will be named "workspaceName-"+spectra\_number

Optionally, it can write the header lines (up to 6) which are preceded with **#** symbol, so do not count as data.
Header provides some metadata information about the file.
When the spectrum axis unit caption of the input workspace is **Temperature**, the corresponding header entry would be preceded with the Fullprof keyword **TEMP**.

Current Issues
--------------

Fullprof expects the data to be in TOF, however at present the
:ref:`algm-DiffractionFocussing` algorithm in Mantid
leaves the data in d-spacing.

If the written file is to be loaded into TOPAS, then headers should be
omitted (set the IncludeHeader property to false);

Usage
-----
**Example - a basic example using SaveFocusedXYE.**

.. testcode:: ExSaveFocusedXYESimple

    import os

    ws = CreateSampleWorkspace()
    ws = ExtractSingleSpectrum(ws, 0)

    file_name = "myworkspace.ascii"
    path = os.path.join(os.path.expanduser("~"), "myworkspace.ascii")

    SaveFocusedXYE(ws, path)
    path = os.path.join(os.path.expanduser("~"), "myworkspace-0.ascii")
    print(os.path.isfile(path))

Output:

.. testoutput:: ExSaveFocusedXYESimple

    True

.. testcleanup:: ExSaveFocusedXYESimple

    import os
    def removeFiles(files):
      for ws in files:
        try:
          path = os.path.join(os.path.expanduser("~"), ws)
          os.remove(path)
        except:
          pass

    removeFiles(["myworkspace-0.ascii"])

**Example - an example using SaveFocusedXYE with additional options.**

.. testcode:: ExSaveFocusedXYEOptions

    import os

    ws = CreateSampleWorkspace()
    ws = CropWorkspace(ws, StartWorkspaceIndex=0, EndWorkspaceIndex=4)

    file_name = "myworkspace.ascii"
    path = os.path.join(os.path.expanduser("~"), file_name)

    SaveFocusedXYE(ws, path, SplitFiles=False, IncludeHeader=True, Format='MAUD')
    print(os.path.isfile(path))


Output:

.. testoutput:: ExSaveFocusedXYEOptions

    True

.. testcleanup:: ExSaveFocusedXYEOptions

    import os
    def removeFiles(files):
      for ws in files:
        try:
          path = os.path.join(os.path.expanduser("~"), ws)
          os.remove(path)
        except:
          pass

    removeFiles([file_name])

.. categories::

.. sourcelink::
