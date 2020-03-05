.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Performs a diffraction reduction for OSIRIS using normalisation to a set of
vanadium sample runs.

The dRanges are numbered as per the `OSIRIS manual
<https://www.isis.stfc.ac.uk/Pages/osiris-user-guide.pdf>`_.
Otherwise the dRange is determined based on the table provided in the manual.

Workflow
--------

.. diagram:: OSIRISDiffractionReduction-v1_wkflw.dot


Usage
-----

**Example - Running OSIRISDiffractionReduction.**

.. testcode:: ExOSIRISDiffractionReductionSimple

    import os
    
    def createDummyOSIRISWorkspace(name, func, xmin, xmax, bin_width):
      """Creates a workspace that looks something like an OSIRIS diffraction run"""
      #create workspace according to function
      ws = CreateSampleWorkspace("Histogram", Function="User Defined", UserDefinedFunction=func, XMin=xmin, XMax=xmax, Random=True, BinWidth=bin_width, NumBanks=11, OutputWorkspace=name)
      ws = CropWorkspace(ws, StartWorkspaceIndex=0, EndWorkspaceIndex=1009, OutputWorkspace=name)
      AddSampleLog(ws, 'gd_prtn_chrg', '30.01270866394043',  'Number')

      #load instrument parameters
      LoadInstrument(ws, RewriteSpectraMap=True, InstrumentName='OSIRIS')
      param_file = config['instrumentDefinition.directory'] + 'OSIRIS_diffraction_diffspec_Parameters.xml'
      LoadParameterFile(ws, Filename=param_file)
      return ws

    #create two dummy workspaces with one peak
    function = "name=Lorentzian, Amplitude=350000,PeakCentre=40000, FWHM=80"
    ws1 = createDummyOSIRISWorkspace('ws1', function, 1.17e+04, 5.17e+04, 4.7)
    ws2 = createDummyOSIRISWorkspace('ws2', function, 29400, 69400, 16.1)

    #create two vanadium runs
    function = "name=FlatBackground, A0=10"
    van1 = createDummyOSIRISWorkspace('van1', function, 1.17e+04, 5.17e+04, 4.7)
    van2 = createDummyOSIRISWorkspace('van2', function, 29400, 69400, 16.1)

    samples = [ws1.name(),  ws2.name() ]
    vanadium = [van1.name(), van2.name()]

    #OSIRISDiffractionReduction currently only support loading from file.
    for ws in (samples + vanadium):
      path = os.path.join(os.path.expanduser("~"), ws + ".nxs")
      SaveNexus(ws, path)

    #run OSIRISDiffractionReduction
    samples = [os.path.join(os.path.expanduser("~"), sample + ".nxs") for sample in samples]
    vanadium = [os.path.join(os.path.expanduser("~"), van  + ".nxs") for van in vanadium]
    ws = OSIRISDiffractionReduction(Sample=','.join(samples), Vanadium=','.join(vanadium), CalFile="osiris_041_RES10.cal")

    print("Number of Spectra: {}, Number of bins: {}".format(ws.getNumberHistograms(), ws.blocksize()))

Output:

.. testoutput:: ExOSIRISDiffractionReductionSimple

    Number of Spectra: 1, Number of bins: 7582

.. testcleanup:: ExOSIRISDiffractionReductionSimple

    import os
    def removeFiles(files):
      for path in files:
        try:
          os.remove(path)
        except:
          pass

    removeFiles(samples)
    removeFiles(vanadium)

.. categories::

.. sourcelink::
