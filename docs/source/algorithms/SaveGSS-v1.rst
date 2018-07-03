.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Saves a focused data set into a three column GSAS format containing
X\_i, Y\_i\*step, and E\_I\*step. Exclusively for the crystallography
package `GSAS <http://www.ccp14.ac.uk/solution/gsas/index.html>`__ .
For data where the focusing routine has generated several spectra (for example, multi-bank instruments), the
option is provided for saving all spectra into a single file, separated
by headers, or into several files that will be named
"workspaceName\_"+workspace\_index\_number.

From the GSAS manual a description of the format options:

-  If BINTYP is 'SLOG' then the neutron TOF data was collected in
   constant ∆T/T steps. BCOEF(1) is the initial TOF in μsec, and
   BCOEF(3) is the value of ∆T/T used in the data collection. BCOEF(2)
   is a maximum TOF for the data set. BCOEF(4) is zero and ignored.
-  If BINTYP equals 'RALF' then the data was collected at one of the TOF
   neutron diffractometers at the ISIS Facility, Rutherford-Appleton
   Laboratory. The width of the time bins is constant for a section of
   the data at small values of TOF and then varies (irregularly) in
   pseudoconstant ∆T/T steps. In this case BCOEF(1) is the starting TOF
   in μsec\*32, BCOEF(2) is the width of the first step in μsec\*32,
   BCOEF(3) is the start of the log scaled step portion of the data in
   μsec\*32 and BCOEF(4) is the resolution to be used in approximating
   the size of each step beyond BCOEF(3).

The format is limited to saving 99 spectra in total. Trying to save more
will generate an error, unless `SplitFiles` is on.

Usage
-----
**Example - a basic example using SaveGSS.**

.. testcode:: ExSaveGSSSimple

    import os
    # Create a workspace to save
    ws = CreateSampleWorkspace(OutputWorkspace="SaveGSSWorkspace")
    ws = ExtractSingleSpectrum(ws, WorkspaceIndex=0)

    # Save to the users home directory
    file_name = "myworkspace.ascii"
    path = os.path.join(os.path.expanduser("~"), file_name)
    SaveGSS(ws, path)

    # Does the file exist
    path = os.path.join(os.path.expanduser("~"), file_name)
    print(os.path.isfile(path))

Output:

.. testoutput:: ExSaveGSSSimple

    True

.. testcleanup:: ExSaveGSSSimple

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

**Example - an example using SaveGSS with additonal options.**

.. testcode:: ExSaveGSSOptions

    import os

    ws = CreateSampleWorkspace(OutputWorkspace="SaveGSSWorkspace")
    # GSAS file cannot have more than 99 entries
    ws = CropWorkspace(ws, StartWorkspaceIndex=0, EndworkspaceIndex=98)

    # Save out GSAS file
    file_name = "myworkspace.ascii"
    path = os.path.join(os.path.expanduser("~"), file_name)
    SaveGSS(ws, path, SplitFiles=False, ExtendedHeader=True, UseSpectrumNumberAsBankID=True)

    print(os.path.isfile(path))

Output:

.. testoutput:: ExSaveGSSOptions

    True

.. testcleanup:: ExSaveGSSOptions

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
