
.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm is meant to adjust the time-of-flight axis so that the elastic peak is at zero energy transfer after unit conversion from 'TOF' to 'DeltaE'. The algorithm has two modes: either it uses a given *ReferenceWorkspace*, or it calculates a constant time-of-flight shift using the L1 + L2 distances of the *InputWorkspace* and incident energy.

Using a reference workspace
###########################

If *ReferenceWorkspace* is set, this algorithm copies the X axis as well as the 'Ei' and 'wavelength' sample logs, if present, to *OutputWorkspace*. The rest of the input properties are discarded.

Calculating new TOF axis
########################

If no *ReferenceWorkspace* is given, the algorithm takes the source-to-sample distance :math:`l_1` from the instrument attached to *InputWorkspace*. The source-to-detector distance :math:`l_2` can either be given directly by the *L2* property, or it is calculated as the mean L2 distance from the histograms specified by *ReferenceSpectra*. The algorithm also needs to know the TOF :math:`t_{elastic}` corresponding to the zero-energy transfer. This is either taken from the first spectrum in *InputWorkspace* as the bin centre of the bin specified by *ElasticBinIndex*, or calculated from the elastic peak positions given in *EPPTable*. *EPPTable* should be in the format returned by the :ref:`algm-FindEPP` algorithm. In this case the algorithm averages the `PeakCentre` column for histograms listed in *ReferenceSpectra*. Finally, the algorithm needs the incident energy :math:`E_i` which can be either specified by *EFixed* or is taken from the sample logs of *InputWorkspace*. In case *EFixed* is specified, the 'Ei' and 'wavelength' sample logs of *OutputWorkspace* are updated accordingly.

The TOF shift :math:`\Delta t` is calculated by

    :math:`\Delta t = t_{elastic} - (l_1 + l_2) / \sqrt{2 E_i / m_n}`,

where :math:`m_n` is the neutron mass. The shift :math:`\Delta t` is then added to all X values of *OutputWorkspace*.

The algorithm assumes micro seconds as units of time and meV as units of energy.

Whether the *ReferenceSpectra* input property refers to workspace indices, spectrum numbers or detector IDs is specified by *IndexType*. 

Usage
-----

**Example - CorrectTOFAxis by specifying the elastic bin and L2**

.. testcode:: ExElasticBinWithL2

    from mantid.kernel import DeltaEModeType, UnitConversion
    
    L1 = 2.0 # in metres.
    L2 = 2.0
    Ei = 55.0 # in meV
    elasticTOF = UnitConversion.run(src='Energy', dest='TOF', 
                                    srcValue=Ei,
                                    l1=L1, l2=L2, 
                                    theta=0, emode=DeltaEModeType.Direct, efixed=Ei)
    
    # Make a workspace with wrong TOF axis.
    TOFMin = 0.0
    TOFMax = 100.0
    binWidth = 0.5
    # Lets say the elastic bin is in the centre of the spectrum.
    elasticBinIndex = int(((TOFMax - TOFMin) / binWidth) / 2.0)
    # Build a Gaussian elastic peak in the workspace.
    peakCentre = TOFMin + elasticBinIndex * binWidth
    spectrumDescription = '''name=Gaussian, PeakCentre={0},
    Height=100, Sigma={1}'''.format(peakCentre, 0.03 * peakCentre)
    ws = CreateSampleWorkspace(WorkspaceType='Histogram',
        NumBanks=1,
        BankPixelWidth=1,
        Function='User Defined',
        UserDefinedFunction=spectrumDescription,
        BankDistanceFromSample=L2,
        SourceDistanceFromSample=L1,
        XMin=TOFMin,
        XMax=TOFMax,
        BinWidth=binWidth)
    
    # Do the correction.
    correctedWs = CorrectTOFAxis(ws,
        IndexType='Workspace Index',
        ElasticBinIndex=elasticBinIndex,
        EFixed=Ei,
        L2=L2)
    
    # Convert TOF to energy transfer.
    convertedWs = ConvertUnits(correctedWs,
        Target='DeltaE',
        EMode='Direct')
    
    # Check results
    # Zero energy transfer should be around elasticBinIndex.
    for index in range(elasticBinIndex-1, elasticBinIndex+2):
        binCentre = (convertedWs.readX(0)[index+1] + convertedWs.readX(0)[index]) / 2
        print('DeltaE at the centre of bin {0}: {1:0.4f}'.format(index,binCentre))

Output:

.. testoutput:: ExElasticBinWithL2

    DeltaE at the centre of bin 99: -0.0893
    DeltaE at the centre of bin 100: -0.0000
    DeltaE at the centre of bin 101: 0.0891

**Example - CorrectTOFAxis by specifying the elastic bin and taking L2 from reference spectra**

.. testcode:: ExElasticBinWithRef

    from mantid.kernel import DeltaEModeType, UnitConversion
    
    L1 = 2.0 # in metres.
    L2 = 2.0
    Ei = 55.0 # in meV
    elasticTOF = UnitConversion.run(src='Energy', dest='TOF', 
                                    srcValue=Ei,
                                    l1=L1, l2=L2, 
                                    theta=0, emode=DeltaEModeType.Direct, efixed=Ei)
    
    # Make a workspace with wrong TOF axis.
    TOFMin = 0.0
    TOFMax = 100.0
    binWidth = 0.5
    # Lets say the elastic bin is in the centre of the spectrum.
    elasticBinIndex = int(((TOFMax - TOFMin) / binWidth) / 2.0)
    # Build a Gaussian elastic peak in the workspace.
    peakCentre = TOFMin + elasticBinIndex * binWidth
    spectrumDescription = '''name=Gaussian, PeakCentre={0},
    Height=100, Sigma={1}'''.format(peakCentre, 0.03 * peakCentre)
    ws = CreateSampleWorkspace(WorkspaceType='Histogram',
        NumBanks=1,
        BankPixelWidth=1,
        Function='User Defined',
        UserDefinedFunction=spectrumDescription,
        BankDistanceFromSample=L2,
        SourceDistanceFromSample=L1,
        XMin=TOFMin,
        XMax=TOFMax,
        BinWidth=binWidth)
    
    # Do the correction.
    correctedWs = CorrectTOFAxis(ws,
        IndexType='Workspace Index',
        ReferenceSpectra='0',
        ElasticBinIndex=elasticBinIndex,
        EFixed=Ei)
    
    # Convert TOF to energy transfer.
    convertedWs = ConvertUnits(correctedWs,
        Target='DeltaE',
        EMode='Direct')
    
    # Check results
    # Zero energy transfer should be around elasticBinIndex.
    for index in range(elasticBinIndex-1, elasticBinIndex+2):
        binCentre = (convertedWs.readX(0)[index+1] + convertedWs.readX(0)[index]) / 2
        print('DeltaE at the centre of bin {0}: {1:0.4f}'.format(index,binCentre))

Output:

.. testoutput:: ExElasticBinWithRef

    DeltaE at the centre of bin 99: -0.0893
    DeltaE at the centre of bin 100: -0.0000
    DeltaE at the centre of bin 101: 0.0891

**Example - CorrectTOFAxis using EPP table**

.. testcode:: ExEPPTable

    from mantid.kernel import DeltaEModeType, UnitConversion
    import numpy
    
    L1 = 2.0 # in metres
    L2 = 2.0
    Ei = 55.0 # in meV
    elasticTOF = UnitConversion.run(src='Energy', dest='TOF', 
                                    srcValue=Ei,
                                    l1=L1, l2=L2, 
                                    theta=0, emode=DeltaEModeType.Direct, efixed=Ei)
    
    # Make a workspace with wrong TOF axis.
    TOFMin = 0.0
    TOFMax = 100.0
    # Build a Gaussian elastic peak in the workspace.
    peakCentre = TOFMin + 2.0 * (TOFMax - TOFMin) / 3.0
    spectrumDescription = '''name=Gaussian, PeakCentre={0},
    Height=100, Sigma={1}'''.format(peakCentre, 0.03 * peakCentre)
    ws = CreateSampleWorkspace(WorkspaceType='Histogram',
           NumBanks=1,
           BankPixelWidth=1,
           Function='User Defined',
           UserDefinedFunction=spectrumDescription,
           BankDistanceFromSample=L2,
           SourceDistanceFromSample=L1,
           XMin=TOFMin,
           XMax=TOFMax,
           BinWidth=0.5)
    
    # Prepare for the correction.
    EPPTable = FindEPP(ws)
    
    # Do the correction.
    correctedWs = CorrectTOFAxis(ws,
        EPPTable=EPPTable,
        IndexType='Workspace Index',
        ReferenceSpectra='0',
        EFixed=Ei)
    
    # Check results.
    print('Original TOF for the elastic peak: {0:0.1f}'.format(
        ws.readX(0)[numpy.argmax(ws.readY(0))]))
    print('Corrected TOF for the elastic peak: {0:0.1f}'.format(
        correctedWs.readX(0)[numpy.argmax(correctedWs.readY(0))]))
    print('Actual elastic TOF: {0:0.1f}'.format(elasticTOF))

Output:

.. testoutput:: ExEPPTable

    Original TOF for the elastic peak: 66.5
    Corrected TOF for the elastic peak: 1232.7
    Actual elastic TOF: 1233.1

**Example - CorrectTOFAxis using a reference workspace**

.. testcode:: ExReferenceWS

    from mantid.kernel import DeltaEModeType, UnitConversion
    import numpy
    
    L1 = 2.0
    L2 = 2.0
    Ei = 55.0 # in meV
    elasticTOF = UnitConversion.run(src='Energy', dest='TOF', 
                                    srcValue=Ei,
                                    l1=L1, l2=L2, 
                                    theta=0, emode=DeltaEModeType.Direct, efixed=Ei)
    
    # Make two workspaces with wrong TOF axis.
    TOFMin = 0.0
    TOFMax = 100.0
    peakCentre = TOFMin + 2.0 * (TOFMax - TOFMin) / 3.0
    # Build a Gaussian elastic peak in the first workspace.
    spectrumDescription = '''name=Gaussian, PeakCentre={0},
    Height=100, Sigma={1}'''.format(peakCentre, 0.03 * peakCentre)
    ws1 = CreateSampleWorkspace(WorkspaceType='Histogram',
        NumBanks=1,
        BankPixelWidth=1,
        Function='User Defined',
        UserDefinedFunction=spectrumDescription,
        BankDistanceFromSample=L2,
        SourceDistanceFromSample=L1,
        XMin=TOFMin,
        XMax=TOFMax,
        BinWidth=0.5)
    # Build a second workspace with slightly different Gaussian.
    spectrumDescription = '''name=Gaussian, PeakCentre={0},
    Height=100, Sigma={1}'''.format(peakCentre, 0.06 * peakCentre)
    ws2 = CreateSampleWorkspace(WorkspaceType='Histogram',
    NumBanks=1,
    BankPixelWidth=1,
    Function='User Defined',
    UserDefinedFunction=spectrumDescription,
    BankDistanceFromSample=L2,
    SourceDistanceFromSample=L1,
    XMin=TOFMin,
    XMax=TOFMax,
    BinWidth=0.5)
    
    # Correct the first workspace using the EPP table method.
    EPPTable = FindEPP(ws1)
    
    # Do the correction.
    correctedWs1 = CorrectTOFAxis(ws1,
        EPPTable=EPPTable,
        IndexType='Workspace Index',
        ReferenceSpectra='0',
        EFixed=Ei)
    
    # Correct the second workspace by using the first as a reference.
    correctedWs2 = CorrectTOFAxis(ws2,
        ReferenceWorkspace=correctedWs1)
    
    # Check results
    print('First workspace original TOF for the elastic peak: {0:0.1f}'.format(
        ws1.readX(0)[numpy.argmax(ws1.readY(0))]))
    print('EPP table corrected TOF for the elastic peak: {0:0.1f}'.format(
        correctedWs1.readX(0)[numpy.argmax(correctedWs1.readY(0))]))
    print('Elastic TOF for the corrected second workspace: {0:0.1f}'.format(
        correctedWs2.readX(0)[numpy.argmax(correctedWs2.readY(0))]))

Output:

.. testoutput:: ExReferenceWS

    First workspace original TOF for the elastic peak: 66.5
    EPP table corrected TOF for the elastic peak: 1232.7
    Elastic TOF for the corrected second workspace: 1232.7

.. categories::

.. sourcelink::
