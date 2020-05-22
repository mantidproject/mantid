.. _02_pim_sol:

============================
Python in Mantid: Exercise 2
============================

Using ISIS Data
===============

Running an analysis Manually
----------------------------

.. code-block:: python


Load(Filename='/Users/danielmurphy/Desktop/Data/TrainingCourseData/LOQ48097.raw', OutputWorkspace='Small_Angle', LoadMonitors='Separate')
ConvertUnits(InputWorkspace='Small_Angle_monitors', OutputWorkspace='Small_Angle_monitors', Target='Wavelength')
ConvertUnits(InputWorkspace='Small_Angle', OutputWorkspace='Small_Angle', Target='Wavelength')
Rebin(InputWorkspace='Small_Angle_monitors', OutputWorkspace='Small_Angle_monitors', Params='2.2,-0.035,10')
Rebin(InputWorkspace='Small_Angle', OutputWorkspace='Small_Angle', Params='2.2,-0.035,10')
ExtractSingleSpectrum(InputWorkspace='Small_Angle_monitors', OutputWorkspace='Small_Angle_monitors', WorkspaceIndex=1)
Divide(LHSWorkspace='Small_Angle', RHSWorkspace='Small_Angle_monitors', OutputWorkspace='Corrected_data')

    # ----------------------------------------------------------------------------
    #  Python Training Exercise 2 Solution with syntax style
    # A generalized script
    #-----------------------------------------------------------------------------

    # Load the monitor spectrum, asking the  user for file
    loadalg = LoadRawDialog(OutputWorkspace="Monitor",SpectrumMin=2,SpectrumMax=2,Message="Enter the raw file you want to process", Disable="SpectrumList")

    # Retrieve the file that was loaded
    file = loadalg.getPropertyValue("Filename")
    # Load the main data bank (same file)
    LoadRaw(Filename=file,OutputWorkspace="Small_Angle",SpectrumMin=130, SpectrumMax=16130)

    # Remove the prompt pulse from the monitor
    RemoveBins(InputWorkspace="Monitor",OutputWorkspace="Monitor",XMin=19900,XMax=20500,Interpolation='Linear')

    # Correct monitor for a flat background
    CalculateFlatBackground(InputWorkspace="Monitor",OutputWorkspace="Monitor",WorkspaceIndexList=0,StartX=31000,EndX=39000)

    # Convert monitor to wavelength
    ConvertUnits(InputWorkspace="Monitor",OutputWorkspace="Monitor",Target="Wavelength")

    # Rebin with a suggested set of parameters
    # The list of parameters [2.2,-0.035,10] can also be given as a '2.2,-0.035,10'
    rebinalg = RebinDialog(InputWorkspace="Monitor",OutputWorkspace="Monitor",Params=[2.2,-0.035,10],Message="Enter the binning you want to use, in wavelength", Enable="Params")
    rebinparam = rebinalg.getPropertyValue("params")

    # Convert data to wavelength
    ConvertUnits(InputWorkspace="Small_Angle",OutputWorkspace="Small_Angle",Target="Wavelength")

    # Rebin the small angle workspace with the same parameters as the previous Rebin
    Rebin(InputWorkspace="Small_Angle",OutputWorkspace="Small_Angle",Params=rebinparam)

    # Finally, correct for incident beam monitor
    Divide(LHSWorkspace="Small_Angle", RHSWorkspace="Monitor",OutputWorkspace="Corrected data")

Using SNS Data
==============

Running an analysis Manually
----------------------------

.. code-block:: python

    Load(Filename=r'EQSANS_6071_event.nxs',OutputWorkspace='run',LoadMonitors='1')

    ConvertUnits(InputWorkspace='run_monitors',OutputWorkspace='run_monitors_lam',Target='Wavelength')
    Rebin(InputWorkspace='run_monitors_lam',OutputWorkspace='run_monitors_lam_rebinned',Params='2.5,0.1,5.5')

    ConvertUnits(InputWorkspace='run',OutputWorkspace='run_lam',Target='Wavelength')
    Rebin(InputWorkspace='run_lam',OutputWorkspace='run_lam_rebinned',Params='2.5,0.1,5.5')

    SumSpectra(InputWorkspace='run_lam_rebinned', OutputWorkspace='run_lam_summed')
    Divide(LHSWorkspace='run_lam_summed', RHSWorkspace='run_monitors_lam_rebinned', OutputWorkspace='normalized')

Generating a script from a workspace
------------------------------------

.. code-block:: python

    load_alg = LoadDialog(OutputWorkspace='run',LoadMonitors='1')
    filename = load_alg.getProperty('Filename').value
    logger.information("Filename is: " + filename)

    ConvertUnits(InputWorkspace='run_monitors',OutputWorkspace='run_monitors_lam',Target='Wavelength')

    rebin_alg = RebinDialog(Message="Rebin the monitors by providing rebin parameters", InputWorkspace='run_monitors_lam',
                        OutputWorkspace='run_monitors_lam_rebinned',Params='2.5,0.1,5.5', Enable='Params')
                        
    params = rebin_alg.getProperty('Params').value

    ConvertUnits(InputWorkspace='run',OutputWorkspace='run_lam',Target='Wavelength')
    Rebin(InputWorkspace='run_lam',OutputWorkspace='run_lam_rebinned',Params=params)

    SumSpectra(InputWorkspace='run_lam_rebinned', OutputWorkspace='run_lam_summed')
    Divide(LHSWorkspace='run_lam_summed', RHSWorkspace='run_monitors_lam_rebinned', OutputWorkspace='normalized')