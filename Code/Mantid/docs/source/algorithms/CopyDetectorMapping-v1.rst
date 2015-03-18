.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm will copy the spectra to detector ID mapping form one workspace
to the other, either on a spectrum index or spectrum number basis.

Typically if both the workspace to be remapped and the workspace being matched
both have their vertical axes in spectrum number then the IndexBySpectrumNumber
option should be considered. With the option disabled the algorithm will copy
the mapping based on the index of the spectrum in the workspace.

Both workspaces must be a :ref:`MatrixWorkspace` with the same number of
histograms.

Usage:
------

**Example: CopyDetectorMapping on generated workspaces**

.. testcode::

    # Create a sample workspace and a workspace to copy mapping to
    to_match = CreateSimulationWorkspace(Instrument='IRIS',
                                         BinParams='-0.5,0.05,0.5')
    to_remap = CreateSampleWorkspace(NumBanks=10, BankPixelWidth=1)

    # Group the spectra in the sample workspace
    grouping_ws = CreateGroupingWorkspace(InstrumentName='IRIS',
                                          FixedGroupCount=10,
                                          ComponentName='graphite')
    to_match = GroupDetectors(InputWorkspace=to_match, PreserveEvents=False,
                              CopyGroupingFromWorkspace='grouping_ws')

    print 'Spectrum 0 detectors before copy: ' + str(to_remap.getSpectrum(0).getDetectorIDs())

    # Copy the grouping to another workspace
    CopyDetectorMapping(WorkspaceToMatch='to_match',
                        WorkspaceToRemap='to_remap',
                        IndexBySpectrumNumber=True)

    print 'Spectrum 0 detectors after copy: ' + str(to_remap.getSpectrum(0).getDetectorIDs())


Output:

.. testoutput::

    Spectrum 0 detectors before copy: set(1)
    Spectrum 0 detectors after copy: set(3,4,5,6,7)

.. categories::
