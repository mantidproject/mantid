.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Conjoin two workspaces, which are file based. Uses
:ref:`algm-ConjoinWorkspaces` to do the heavy-lifting.

The files are expected to be in a GSAS format, loadable by LoadGSS, with a .gsa or .txt extension.

.. hint:: The filename is expected to be of the format:
          [instrument]_[run_no].[extension]

Usage
-----

**ConjoinFiles Example**

.. testcode:: ConjoinFilesEx

    import os

    #Create an absolute path by joining the proposed filename to a directory
    #os.path.expanduser("~") used in this case returns the home directory of the current user
    savePath = os.path.expanduser("~")
    ws1Path = os.path.join(savePath, ConfigService.getInstrument().shortName() + "_1234.gsa")
    ws2Path = os.path.join(savePath, ConfigService.getInstrument().shortName() + "_4567.gsa")

    ws1 = CreateSampleWorkspace(WorkspaceType="Histogram", NumBanks=2, BankPixelWidth=1, BinWidth=10, Xmax=50)
    print("Number of spectra in first workspace {}".format(ws1.getNumberHistograms()))
    SaveGSS(ws1, ws1Path, SplitFiles=False, Append=False)

    ws2 = CreateSampleWorkspace(WorkspaceType="Histogram", NumBanks=2, BankPixelWidth=1, BinWidth=10, Xmax=50)
    print("Number of spectra in second workspace {}".format(ws2.getNumberHistograms()))
    SaveGSS(ws2,ws2Path, SplitFiles=False, Append=False)

    wsOutput = ConjoinFiles(RunNumbers=[1234,4567], Directory= savePath)
    print("Number of spectra after ConjoinWorkspaces {}".format(wsOutput.getNumberHistograms()))

    os.remove(ws1Path)
    os.remove(ws2Path)

Output:

.. testoutput:: ConjoinFilesEx

    Number of spectra in first workspace 2
    Number of spectra in second workspace 2
    Number of spectra after ConjoinWorkspaces 4


.. categories::

.. sourcelink::
