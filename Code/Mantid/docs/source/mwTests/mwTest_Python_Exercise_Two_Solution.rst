:orphan:

.. Skipping Test  mwTest_Python_Exercise_Two_Solution[4]


.. testcode:: mwTest_Python_Exercise_Two_Solution[44]

   Load(Filename=r'EQSANS_6071_event.nxs',OutputWorkspace='run',LoadMonitors='1')
   
   ConvertUnits(InputWorkspace='run_monitors',OutputWorkspace='run_monitors_lam',Target='Wavelength')
   Rebin(InputWorkspace='run_monitors_lam',OutputWorkspace='run_monitors_lam_rebinned',Params='2.5,0.1,5.5')
   
   ConvertUnits(InputWorkspace='run',OutputWorkspace='run_lam',Target='Wavelength')
   Rebin(InputWorkspace='run_lam',OutputWorkspace='run_lam_rebinned',Params='2.5,0.1,5.5')
   
   SumSpectra(InputWorkspace='run_lam_rebinned', OutputWorkspace='run_lam_summed')
   Divide(LHSWorkspace='run_lam_summed', RHSWorkspace='run_monitors_lam_rebinned', OutputWorkspace='normalized')


.. Skipping Test  mwTest_Python_Exercise_Two_Solution[59]


