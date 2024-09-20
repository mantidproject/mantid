.. _project_recovery_testing:

Project Recovery Testing
=========================

.. contents::
  :local:

Project Recovery test
---------------------

*Preparation*

- Before running these tests, open ``File > Settings > General > Project Recovery`` and set ``Enabled`` to true,
  ``Time between recovery checkpoints`` to 2 seconds and ``Total number of checkpoints`` to 5.
  Further instructions can be found on the
  :ref:`Project Recovery concepts page <Project Recovery>`.
- Download the ISIS sample dataset from the `Downloads page <http://download.mantidproject.org/>`_.
- The files ``INTER000*`` and ``SXD23767.raw`` are in the ISIS sample data.
- Include the directory containing the test files in your Managed User Directories.
- Set your facility to ISIS.
- Set up a save directory to store output for comparison, referred to as ``testing_directory`` below.
- Note that if you have error reporting enabled, simply select ``Do not share information`` in the Error Reporter dialog.
- Restart Workbench to ensure all changes are applied.


**Time required 15 - 30  minutes**

--------------

1. Simple tests

- Open MantidWorkbench
- Right-click in the Messages Box and set `Log level` to `Debug`
- Currently, all that should be printed is `Nothing to save`
- Run the following command to create a simple workspace:

.. code-block:: python

  CreateWorkspace(DataX=range(12), DataY=range(12), DataE=range(12), NSpec=4, OutputWorkspace='NewWorkspace')

- The Messages box should now be printing `Project Recovery: Saving started` and `Project Recovery: Saving finished` on
  alternate lines
- Now run this script:

.. code-block:: python

   Load(Filename='INTER00013464.nxs', OutputWorkspace='INTER1')
   Load(Filename='INTER00013469.nxs', OutputWorkspace='INTER2')
   Load(Filename='INTER00013469.nxs', OutputWorkspace='INTER3')
   RenameWorkspace(InputWorkspace='INTER2', OutputWorkspace='Rename2')
   RenameWorkspace(InputWorkspace='INTER1', OutputWorkspace='Rename1')
   RenameWorkspace(InputWorkspace='INTER3', OutputWorkspace='Rename3')
   Fit(Function='name=DynamicKuboToyabe,BinWidth=0.05,' 'Asym=5.83382,Delta=5.63288,Field=447.873,Nu=8.53636e-09', InputWorkspace='Rename1', IgnoreInvalidData=True, Output='Rename1_fit', OutputCompositeMembers=True, ConvolveMembers=True)
   Fit(Function='name=ExpDecayMuon,A=4306.05,Lambda=0.458289', InputWorkspace='Rename2', IgnoreInvalidData=True, Output='Rename2_fit', OutputCompositeMembers=True, ConvolveMembers=True)
   Fit(Function='name=Abragam,A=-500.565,Omega=944.105,Phi=-2.97876,Sigma=230.906,Tau=5.54415e+06', InputWorkspace='Rename1_fit_Workspace', CreateOutput=True, Output='Rename1_fit_Workspace_1', CalcErrors=True)
   Fit(Function='name=Abragam,A=343210,Omega=-91853.1,Phi=-1.51509,Sigma=11920.5,Tau=2.80013e+13', InputWorkspace='Rename2_fit_Workspace', CreateOutput=True, Output='Rename2_fit_Workspace_1', CalcErrors=True)
   GroupWorkspaces(InputWorkspaces='Rename1_fit_Workspace_1_Workspace,Rename2_fit_Workspace_1_Workspace', OutputWorkspace='Rename3_fit_Workspaces')
   RenameWorkspace(InputWorkspace='Rename1_fit_Workspace_1_Workspace', OutputWorkspace='Sequential1')
   RenameWorkspace(InputWorkspace='Rename2_fit_Workspace_1_Workspace', OutputWorkspace='Sequential2')
   Fit(Function='name=ExpDecayMuon,A=4306.05,Lambda=0.458289', InputWorkspace='Rename3', IgnoreInvalidData=True, Output='Rename3_fit', OutputCompositeMembers=True, ConvolveMembers=True)
   Fit(Function='name=ExpDecayMuon,A=4306.05,Lambda=0.458289', InputWorkspace='Rename2_fit_Workspace', CreateOutput=True, Output='Rename2_fit_Workspace_1', CalcErrors=True)
   Fit(Function='name=ExpDecayMuon,A=4306.05,Lambda=0.458289', InputWorkspace='Rename3_fit_Workspace', CreateOutput=True, Output='Rename3_fit_Workspace_1', CalcErrors=True)
   GroupWorkspaces(InputWorkspaces='Rename2_fit_Workspace_1_Workspace,Rename3_fit_Workspace_1_Workspace', OutputWorkspace='Rename3_fit_Workspaces')
   RenameWorkspace(InputWorkspace='Rename3_fit_Workspace_1_Workspace', OutputWorkspace='Sequential3')
   RenameWorkspace(InputWorkspace='Rename2_fit_Workspace_1_Workspace', OutputWorkspace='Sequential4')
   Fit(Function='name=ExpDecayMuon,A=4306.05,Lambda=0.458289', InputWorkspace='Rename3_fit_Workspace', CreateOutput=True, Output='Rename3_fit_Workspace_1', CalcErrors=True)
   Fit(Function='name=ExpDecayMuon,A=4306.05,Lambda=0.458289', InputWorkspace='Rename1_fit_Workspace', CreateOutput=True, Output='Rename1_fit_Workspace_1', CalcErrors=True)
   GroupWorkspaces(InputWorkspaces='Rename3_fit_Workspace_1_Workspace,Rename1_fit_Workspace_1_Workspace', OutputWorkspace='Rename3_fit_Workspaces')
   RenameWorkspace(InputWorkspace='Rename3_fit_Workspace_1_Workspace', OutputWorkspace='Sequential5')
   RenameWorkspace(InputWorkspace='Rename1_fit_Workspace_1_Workspace', OutputWorkspace='Sequential6')

- Wait a few seconds, then provoke a crash by executing the `Segfault` algorithm with ``DryRun`` set to False.
- Restart MantidWorkbench
- You should be presented with the Project Recovery dialog
- Choose `Yes`
- This should re-populate your workspace dialog and pop up a recovery script in the script window

--------

2. Testing many workspaces

- Open up MantidWorkbench
- Run the following script:

.. code-block:: python

   testing_directory='<path-to-test>'
   # <path-to-test> is the location of a directory for saving workspaces for comparison later
   # e.g. C:\Users\abc1234\Desktop\test_proj_rec\
   CreateWorkspace(DataX=range(12), DataY=range(12), DataE=range(12), NSpec=4, OutputWorkspace='0Rebinned')
   for i in range(100):
       RenameWorkspace(InputWorkspace='%sRebinned'%str(i), OutputWorkspace='%sRebinned'%str(i+1))
   for i in range(300):
       CloneWorkspace(InputWorkspace='100Rebinned', OutputWorkspace='%sClone'%str(i))
   SaveCSV(InputWorkspace='299Clone', Filename=testing_directory + 'Clone.csv')

- Wait a few seconds, then provoke a crash by executing the `Segfault` algorithm
- Restart MantidWorkbench
- You should be presented with the Project Recovery dialog
- Choose `Yes`
- This should re-populate your workspace dialog and pop up a recovery script in the script window
- Run the following script:

.. code-block:: python

   testing_directory='<path-to-test>'
   SaveCSV(InputWorkspace='299Clone', Filename=testing_directory +'Clone_r.csv')

- Compare the contents of `Clone.csv` and `Clone_r.csv`, they should be the same

------

3. Testing workspaces of different types

- Open up MantidWorkbench
- Run the following script:

.. code-block:: python

   testing_directory='<path-to-test>'
   Load(Filename=r'SXD23767.raw', OutputWorkspace='SXD23767')
   ConvertToDiffractionMDWorkspace(InputWorkspace='SXD23767', OutputWorkspace='SXD23767_MD', OneEventPerBin=False, SplitThreshold=30)
   DeleteWorkspace("SXD23767")
   multi_d = RenameWorkspace('SXD23767_MD')
   peaks = FindPeaksMD(InputWorkspace='multi_d', PeakDistanceThreshold=0.4, MaxPeaks=10,
           PeakFindingStrategy='NumberOfEventsNormalization', SignalThresholdFactor=10,
           OutputType='Peak', OutputWorkspace='SingleCrystalPeakTable', EdgePixels=1)

   long1=CreateMDHistoWorkspace(Dimensionality=2, Extents='-3,3,-10,10', SignalInput=range(0,10000), ErrorInput=range(0,10000),\
                           NumberOfBins='100,100', Names='Dim1,Dim2', Units='MomentumTransfer, EnergyTransfer')

   long2=CreateMDHistoWorkspace(Dimensionality=2, Extents='-3, 3, -10, 10', SignalInput=range(0, 10000), ErrorInput=range(0, 10000),\
                           NumberOfBins='100, 100', Names='Dim1, Dim2', Units='MomentumTransfer, EnergyTransfer')
   long3=long1+long2
   DeleteWorkspace("long1")
   DeleteWorkspace("long2")
   long4=long3.clone()
   DeleteWorkspace("long3")
   CloneWorkspace(InputWorkspace='long4', OutputWorkspace='Clone')
   ConvertMDHistoToMatrixWorkspace(InputWorkspace='Clone', OutputWorkspace='Clone_matrix')
   SaveCSV('Clone_matrix' , testing_directory + '/method_test.csv')

   DgsReduction(SampleInputFile='MAR11001.raw', IncidentEnergyGuess=12, OutputWorkspace='ws')
   Rebin(InputWorkspace='ws', OutputWorkspace='rebin', Params='0.5')
   Rebin(InputWorkspace='rebin', OutputWorkspace='rebin', Params='0.6')
   Rebin(InputWorkspace='rebin', OutputWorkspace='rebin', Params='0.7')
   Rebin(InputWorkspace='rebin', OutputWorkspace='rebin', Params='0.8')
   RenameWorkspace(InputWorkspace='rebin', OutputWorkspace='renamed')
   SaveCSV('renamed', testing_directory + '/rebin_test.csv')


   long4 *= 4
   long4 += 3.00
   ConvertMDHistoToMatrixWorkspace(InputWorkspace='long4', OutputWorkspace='long4_matrix')
   SaveCSV('long4_matrix', testing_directory + '/test_binary_operators.csv')

- Force a crash by executing the `Segfault` algorithm
- Restart MantidWorkbench
- You should be presented with the Project Recovery dialog
- Choose `Yes`

.. code-block:: python

    testing_directory='<path-to-test>'
    SaveCSV('Clone_matrix' , testing_directory + '/method_test_r.csv')
    SaveCSV('long4_matrix', testing_directory + '/test_binary_operators_r.csv')

- Compare the contents of ``/test_binary_operators.csv`` and ``/test_binary_operators_r.csv``, they should be the same
- Compare the contents of ``/method_test.csv`` and ``/method_test_r.csv``, they should be the same

--------

4. Recovering plots and windows

- Open MantidWorkbench - make sure no other instances of MantidWorkbench are running
- Run the large script from test 1
- In the workspace window right-click the ``Sequential3`` workspace and choose `Plot spectrum`
- Choose `Plot All`
- In the workspace window right-click the ``Sequential1`` workspace and choose `Plot spectrum`
- Change Plot type from individual to `Tiled`, and again click `Plot all`
- In the workspace window right-click the ``Rename2`` workspace and select `Show Data`
- In the top toolbar, navigate to ``Interfaces > Reflectometry`` and open the ``ISIS Reflectometry`` interface
- In the top toolbar, navigate to ``Interfaces > Diffraction`` and open the ``Engineering Diffraction`` interface.

.. image:: ../../images/reporter-test-4.png


- Force a crash by executing the `Segfault` algorithm
- Restart MantidWorkbench
- You should be presented with the Project Recovery dialog
- Choose `Yes`
- Mantid should reload the workspaces and reopen plots and interfaces (including the show data interface).
  You should see these all reappear in the main screen (they may have been reopened, but minimised).

*(Note at time of writing, only ISIS Reflectometry and Engineering Diffraction are supported by Project Save / Recovery)*

---------

5. Test multiple instances of Mantid running

- Launch 2 instances of MantidWorkbench
- Run the script on the first instance:

.. code-block:: python

  CreateWorkspace(DataX=range(12), DataY=range(12), DataE=range(12), NSpec=4, OutputWorkspace='Instance 1')

- Run this script on the other instance:

.. code-block:: python

  CreateWorkspace(DataX=range(12), DataY=range(12), DataE=range(12), NSpec=4, OutputWorkspace='Instance 2')

- Crash the first instance of Mantid with `Segfault`
- Do not exit the second instance of Mantid
- Restart MantidWorkbench
- You should be presented with a Project Recovery dialog, offering to attempt a recovery - choose `Yes`
- `Instance 1` should appear in the workspace dialog

---------

6. Opening script only

- Open MantidWorkbench
- Run the large script from test 1
- In the workspace window right-click the ``Sequential3`` workspace and choose `Plot spectrum`
- Choose `Plot All`
- Force a crash by executing the `Segfault` algorithm
- Restart MantidWorkbench
- You should be presented with the Project Recovery dialog
- Choose ``Just open in script editor``
- Mantid should open the script editor, with a script named `ordered_recovery.py`
- Run this script, it should repopulate the workspaces dialog, but not open any figures

---------

7. Not attempting recovery

- Open MantidWorkbench
- Run the second script from test 1
- In the workspace window right-click the ``Sequential3`` workspace and choose `Plot spectrum`
- Choose `Plot All`
- Force a crash by executing the `Segfault` algorithm
- Restart MantidWorkbench
- You should be presented with the Project Recovery dialog
- Choose ``Start mantid normally``
- Mantid should open as normal
- With the Messages box at Debug level you should see the project saver starting up again

---------

8. Check old history is purged

- Open MantidWorkbench

.. code-block:: python

  CreateWorkspace(DataX=range(12), DataY=range(12), DataE=range(12), NSpec=4, OutputWorkspace='NewWorkspace')
  RenameWorkspace(InputWorkspace='NewWorkspace', OutputWorkspace='Rename2')

- Save the workspace as a `.nxs` file, by highlighting the ``Rename2`` workspace and selecting
  ``Save Nexus`` at the top of the Workspaces toolbox.
- Close Mantid normally
- Restart MantidWorkbench
- Re-open the workspace from the saved `.nxs` file
- Wait for saving
- Force a crash by executing the `Segfault` algorithm
- Restart MantidWorkbench
- Choose ``Just open in script editor``
- Mantid should open a script named ``ordered_recovery.py`` in the script editor
- This should contain only the ``Load`` command and no previous history (to see full history, run the script, right-click on the
  workspace and select ``Show History``)

Finally, test out a few ideas of your own. Note that some more niche aspects of plotting are not saved, such as 3D plots,
and Sliceviewer is also not supported by project save/recovery.

**Complete!** Thank you for testing! Make sure to **raise any issues** you found on Github.
