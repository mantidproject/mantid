.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Saves a workspace with momentum transfer units into a file adhering to the NXcanSAS format specified by NXcanSAS Data
Formats Working Group `schema <http://cansas-org.github.io/NXcanSAS/classes/contributed_definitions/NXcanSAS.html>`__.

1D or 2D workspaces may be saved.


If the input workspace is a group, every compatible workspace member of the group will be saved in a separate file.

In addition, it is possible to save the transmission workspaces obtained from a reduction.


Usage
-----

**Example**

.. testcode:: SavePolarizedNXcanSAS

    workspaces = []
    #Create sample workspace group with simple data.
    for n in range(4):
        ws = CreateSampleWorkspace(OutputWorkspace=f'out_{n}', Function='User Defined', UserDefinedFunction=f'name=Lorentzian, Amplitude=1000, PeakCentre={n+1}, FWHM=1', XUnit='MomentumTransfer', NumBanks=1, BankPixelWidth=1, XMin=0, XMax=16.5, BinWidth=0.1)
        LoadInstrument(f'out_{n}',InstrumentName='ZOOM', RewriteSpectraMap=False)
        workspaces.append(ws)

    group = GroupWorkspaces(workspaces)
    SavePolarizedNXcanSAS(group,'test_file', InputSpinStates='+1-1,-1-1,+1+1,-1+1')

.. categories::

.. sourcelink::
