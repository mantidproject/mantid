.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

The fluctuation dissipation theorem [1,2] relates the dynamic
susceptibility to the scattering function

:math:`\left(1-e^{-\frac{E}{k_B T}}\right) S(\mathbf{q}, E) = \frac{1}{\pi} \chi'' (\mathbf{q}, E)`

For the MD events, the detailed balance is calculated and applied as below.

:math:`F = \pi(1-e^{-\frac{\Delta E}{k_B T(i)}})`

:math:`I *= F`

:math:`Err^2 *= F^2`


where :math:`E` is the energy transfer to the system. The algorithm
assumes that the signal of the input workspace contains the scattering
function :math:`S`. The signal of the output workspace will contain the
dynamic susceptibility. The temperature is either extracted as the average of the
values stored in the appropriate entry of the log attached to the workspace
(user supplies the name of the entry) or user can pass a number for the temperature.

One can use this formula to calculate the scattering at different temperature:

:math:`\left(1-e^{-\frac{E}{k_B T_1}}\right) S(\mathbf{q}, E, T_1)=\left(1-e^{-\frac{E}{k_B T_2}}\right) S(\mathbf{q}, E, T_2)`


[1] S. W. Lovesey - Theory of Neutron Scattering from Condensed Matter,
vol 1

[2] I. A. Zaliznyak and S. H. Lee - Magnetic Neutron Scattering in
"Modern techniques for characterizing magnetic materials"

Usage
-----

**Example - Run Applied Detailed Balance**

.. testcode:: ExApplyDetailedBalanceMDSimple

   we1 = CreateSampleWorkspace(WorkspaceType='Event',
                              Function='Flat background',
                              BankPixelWidth=1,
                              XUnit='DeltaE',
                              XMin=-10,
                              XMax=19,
                              BinWidth=0.5)

   AddSampleLog(Workspace=we1,LogName='Ei', LogText='20.', LogType='Number')
   MoveInstrumentComponent(Workspace=we1, ComponentName='bank1', X=3, Z=3, RelativePosition=False)
   MoveInstrumentComponent(Workspace=we1, ComponentName='bank2', X=-3, Z=-3, RelativePosition=False)
   AddSampleLog(Workspace=we1,LogName='SampleTemp', LogText='25.0', LogType='Number Series')
   SetGoniometer(Workspace=we1, Axis0='0,0,1,0,1')

   # old way
   weadb1 = ApplyDetailedBalance(InputWorkspace=we1, Temperature='SampleTemp')
   mdabd1 = ConvertToMD(InputWorkspace=weadb1, QDimensions='Q3D')

   # use algorithm ApplyDetailedBalanceMD
   md1 = ConvertToMD(InputWorkspace=we1, QDimensions='Q3D')
   test_db_md = ApplyDetailedBalanceMD(md1, 'SampleTemp')

   r = CompareMDWorkspaces(test_db_md, mdabd1, CheckEvents=True, Tolerance=0.00001)
   print('Number of MDEvents: {} == {}'.format(test_db_md.getNEvents(), mdabd1.getNEvents()))
   print('Workspaces are {} equal'.format(r.Equals))

Output:

.. testoutput:: ExApplyDetailedBalanceMDSimple

   Number of MDEvents: 1972 == 1972
   Workspaces are True equal

.. categories::

.. sourcelink::
