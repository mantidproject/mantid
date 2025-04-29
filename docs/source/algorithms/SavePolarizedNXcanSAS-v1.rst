.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Saves a workspace with momentum transfer units into a file adhering to the NXcanSAS format specified by NXcanSAS Data
Formats Working Group `schema <http://cansas-org.github.io/NXcanSAS/classes/contributed_definitions/NXcanSAS.html>`__.

This algorithm in particular, adheres to the WIP `proposal <https://wiki.cansas.org/index.php?title=NXcanSAS_v1.1>`__ to
update the format to include reduced SANS polarized data.

The algorithm is an extension of :ref:`algm-SaveNXcanSAS`, all the metadata available in :ref:`algm-SaveNXcanSAS` can be saved
here too.

The :literal:`InputWorkspace` property must be a Workspace Group containing as many members as measured polarization states,
which typically would be 2 members for half-polarized SANS measurements and 4 members for full-polarized SANS.

The spin states are set through the :literal:`InputSpinStates` property, every polarization configuration is defined as a *PinPout* pair, with
the possible values for each spin state before (*Pin*) or after (*Pout*) the sample being "+1", "-1" or "0" as defined in the `proposal <https://wiki.cansas.org/index.php?title=NXcanSAS_v1.1>`__.
There must be as many *PinPout* pairs as workspace members in the :literal:`InputWorkspace` property, in the same order of polarization
as that on the workspaces. For example, a full polarized measurement could be defined in the :literal:`InputSpinStates` property
as *"+1+1,-1+1,-1-1,-1-1"* , with *"-1-1"* and *"+1+1"* being the non spin-flip configurations and *"-1+1 , +1-1"* the spin-flip ones.

In addition, the magnetic field strength can be added by referring to the sample log of the :literal:`InputWorkspace` containing the data in :literal:`MagneticFieldStrengthLogName`,
the direction of the magnetic field in spherical coordinates is specified in the :literal:`MagneticFieldDirection` property by setting
the dimensions of the spherical vector (polar, azimuthal, rotation angles) as comma separated values.

The information for the polarizing components (polarizer, flippers, analyzer) must be specified in :literal:`PolarizerComponentName`, :literal:`FlipperComponentNames`, :literal:`AnalyzerComponentName`
properties by writing the polarizer component name as defined in the corresponding Instrument Definition File (IDF). For each component; name, type and distance to sample
will be written in the the corresponding dataset. If there is more than one flipper, they can be added by their IDF component name as comma separated lists in the :literal:`FlipperComponentNames` property.


Usage
-----

**Example**

.. testcode:: SavePolarizedNXcanSAS

    #Create sample workspace group with simple test data and adding ZOOM instrument.
    names = []
    for n in range(4):
        name = f"out_{n}"
        CreateSampleWorkspace(OutputWorkspace=name, Function="User Defined", UserDefinedFunction=f"name=Lorentzian, Amplitude=1000, PeakCentre={n+1}, FWHM=1", XUnit="MomentumTransfer", NumBanks=1, BankPixelWidth=1, XMin=0, XMax=16.5, BinWidth=0.1)
        LoadInstrument(name,InstrumentName="ZOOM", RewriteSpectraMap=False)
        names.append(name)

    group = GroupWorkspaces(names)
    SavePolarizedNXcanSAS(group, "test_file", InputSpinStates="+1-1,-1-1,+1+1,-1+1")

.. categories::

.. sourcelink::
