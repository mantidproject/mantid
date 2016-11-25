
.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm is meant to adjust the time-of-flight axis so that the elastic peak is at zero energy transfer after unit conversion from 'TOF' to 'DeltaE'. The algorithm has two modes: either it uses a given *ReferenceWorkspace*, or it calculates a constant time-of-flight shift using the L1 + L2 distances of the *InputWorkspace* and incident energy.

If *ReferenceWorkspace* is set, this algorithm copies the X axis as well as the 'Ei' and 'wavelength' sample logs to the *OutputWorkspace*. The rest of the input properties are discarded.

If no *ReferenceWorkspace* is given, the algorithm calculates the average L2 distance and the average elastic peak positions. The L2 values it gets from the *InputWorkspace* while the elastic peak positions are taken from *EPPTable*. *EPPTable* should be in the format returned by the :ref:`algm-FindEPP` algorithm. The spectra over which the averages are taken should be listed in *ReferenceSpectra*. Whether this input property refers to workspace indices, spectrum numbers or detector IDs is specified by *IndexType*. Incident energy can be either specified by *IncidentEnergy* or it is taken from the sample logs of *InputWorkspace*. In case *IncidentEnergy* is specified, the 'Ei' and 'wavelength' sample logs of *OutputWorkspace* are updated accordingly.

Usage
-----

**Example - TOFAxisCorrection**

.. testcode:: TOFAxisCorrectionExample

   import numpy
   from scipy import constants
   
   L1 = 2.0
   L2 = 2.0
   Ei = 55.0 # in meV
   v = numpy.sqrt(2 * Ei * 1e-3 * constants.e / constants.m_n)
   elasticTOF = (L1 + L2) / v * 1e6 # in micro seconds.
   
   # Make a workspace with wrong TOF axis.
   TOFMin = 0.0
   TOFMax = 100.0
   peakCentre = TOFMin + 2.0 * (TOFMax - TOFMin) / 3.0
   # Build a Gaussian elastic peak in the workspace.
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
   correctedWs = TOFAxisCorrection(ws,
       EPPTable=EPPTable,
       IndexType='Workspace Index',
       ReferenceSpectra='0',
       IncidentEnergy=Ei)
   
   # Check results.
   print('Original TOF for the elastic peak: {0:0.1f}'.format(
      ws.readX(0)[numpy.argmax(ws.readY(0))]))
   print('Corrected TOF for the elastic peak: {0:0.1f}'.format(
      correctedWs.readX(0)[numpy.argmax(correctedWs.readY(0))]))
   print('Actual elastic TOF: {0:0.1f}'.format(elasticTOF))

Output:

.. testoutput:: TOFAxisCorrectionExample

   Original TOF for the elastic peak: 66.5
   Corrected TOF for the elastic peak: 1232.7
   Actual elastic TOF: 1233.1

.. categories::

.. sourcelink::
