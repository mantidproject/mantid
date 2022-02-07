.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

In instruments where there is a polarization analysis stage in the scattered beam, neutron intensity is reduced by
an absorption factor [1]. This has an exponential dependence on the scattered energy. To correct for this,
the detected intensity :math:`I` of every event collected is corrected by:

:math:`E_f = E_i-\Delta E`

:math:`I' = I e^{c E_f}`

:math:`\delta I' = \delta I e^{c E_f}`

[1]  Andrei T. Savici et al, 2017, J. Phys.: Conf. Ser. 862 012023

Usage
-----

**Example - Run Transmission Correction**

.. testcode:: DgsScatteredTransmissionCorrectionSingleRun

   we1 = CreateSampleWorkspace(WorkspaceType='Event',
                               Function='Flat background',
                               BankPixelWidth=1,
                               XUnit='DeltaE',
                               XMin=-10,
                               XMax=19,
                               BinWidth=0.5)
   AddSampleLog(Workspace=we1,LogName='deltaE-mode', LogText='Direct', LogType='String')
   AddSampleLog(Workspace=we1,LogName='Ei', LogText='20.', LogType='Number')
   MoveInstrumentComponent(Workspace=we1, ComponentName='bank1', X=3, Z=3, RelativePosition=False)
   MoveInstrumentComponent(Workspace=we1, ComponentName='bank2', X=-3, Z=-3, RelativePosition=False)
   SetGoniometer(Workspace=we1, Axis0='0,0,1,0,1')

   c = 1. / 11  # the exponential factor for the correction

   # old way
   we1s = ScaleX(InputWorkspace=we1, Factor=-20., Operation='Add') # deltaE - Ei becomes -Ef
   we1s = ScaleX(InputWorkspace=we1s, Factor=-1.0, Operation='Multiply') # the X-axis becomes Ef
   # The ExponentialCorrection algorithm multiplies signal by C0*exp(-C1*x), where x is Ef
   we1s = ExponentialCorrection(Operation="Multiply", InputWorkspace=we1s, C0=1., C1=-c)
   we1s = ScaleX(InputWorkspace=we1s, Factor=-1.0, Operation='Multiply') # the X-axis becomes -Ef
   we1s = ScaleX(InputWorkspace=we1s, Factor=20., Operation='Add') # Ei - Ef converts back to DeltaE
   md1_old = ConvertToMD(InputWorkspace=we1s, QDimensions='Q3D')

   # use algorithm DgsScatteredTransmissionCorrectionMD
   md1 = ConvertToMD(InputWorkspace=we1, QDimensions='Q3D')
   md1_new = DgsScatteredTransmissionCorrectionMD(md1, ExponentFactor=c)

   r = CompareMDWorkspaces(md1_old, md1_new, CheckEvents=True, Tolerance=0.00001)
   print('Number of MDEvents: {} == {}'.format(md1_old.getNEvents(), md1_new.getNEvents()))
   print('md1_old and md1_new being equal is {}'.format(r.Equals))

Output:

.. testoutput:: DgsScatteredTransmissionCorrectionSingleRun

   Number of MDEvents: 1972 == 1972
   md1_old and md1_new being equal is True

.. categories::

.. sourcelink::
