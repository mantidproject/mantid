.. _project_recovery_testing:

Project Recovery Testing
=========================

.. contents::
  :local:

Project Recovery test
---------------------

*Preparation*

- Before running these tests, set project recovery to run every 2 seconds. The instructions for this
  are on the `Project Recovery concepts page <http://docs.mantidproject.org/nightly/concepts/ProjectRecovery.html>`_.
- Get the ISIS sample dataset from the `Downloads page <http://download.mantidproject.org/>`_.
- Put the data in a data directory, referred to as ``data_directory`` in the scripts below
- Set up a directory to store output for comparison, referred to as ``testing_directory`` below


**Time required XX - YY  minutes**

--------------

1. Simple tests and Muon-esque workflow

- Open MantidPlot - make sure no other instances of MantidPlot are running
- Right-click in the Results Log and set `Log level` to `Debug`
- The Results Log should be printing `Nothing to save`
- Run the following command to create a simple workspace:

.. code-block:: python

  CreateWorkspace(DataX=range(12), DataY=range(12), DataE=range(12), NSpec=4, OutputWorkspace='NewWorkspace')

- The Results Log should now be printing `Project Recovery: Saving started` and `Project Recovery: Saving finished` on alternate lines
- Now run this script:

.. code-block:: python

   data_directory=<path-to-data>
   testing_directory=<path-to-test>
   Load(Filename=data_directory + '/INTER00013464.nxs', OutputWorkspace='INTER1')
   Load(Filename=data_directory + '/INTER00013469.nxs', OutputWorkspace='INTER2')  
   Load(Filename=data_directory + '/INTER00013469.nxs', OutputWorkspace='INTER3')  
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
   SaveCSV(InputWorkspace='Sequential4', Filename=testing_directory + '/Sequence4.csv')
   SaveCSV(InputWorkspace='Sequential5', Filename=testing_directory + '/Sequence5.csv')
   SaveCSV(InputWorkspace='Sequential6', Filename=testing_directory + '/Sequence6.csv') 

- Wait a few seconds, then provoke a crash by running `Segfault`
- Re-start MantidPlot
- You should be presented with the Project Recovery dialog
- Choose `Yes`
- This should re-populate your workspace dialog and pop up a recovery script in the script window
- Run the following script:

.. code-block:: python

    SaveCSV(InputWorkspace='Sequential4', Filename=testing_directory + '/Sequence4.csv')
    SaveCSV(InputWorkspace='Sequential5', Filename=testing_directory + '/Sequence5.csv')
    SaveCSV(InputWorkspace='Sequential6', Filename=testing_directory + '/Sequence6.csv')

- Compare the contents of the `SequenceX.csv` and `SequenceXr.csv` files, they should be the same

-------- 

2. Testing many workspaces

- Open up MantidPlot, ensure that it is the only instance running
- Run the following script:

.. code-block:: python

   CreateWorkspace(DataX=range(12), DataY=range(12), DataE=range(12), NSpec=4, OutputWorkspace='0Rebinned')
   for i in range(100):
       RenameWorkspace(InputWorkspace='%sRebinned'%str(i), OutputWorkspace='%sRebinned'%str(i+1))
   for i in range(3000):
       CloneWorkspace(InputWorkspace='100Rebinned', OutputWorkspace='%sClone'%str(i))
   SaveCSV(InputWorkspace='2999Clone', Filename='Clone.csv')

- Wait a few seconds, then provoke a crash by running `Segfault`
- Re-start MantidPlot
- You should be presented with the Project Recovery dialog
- Choose `Yes`
- This should re-populate your workspace dialog and pop up a recovery script in the script window
- Run the following script:

.. code-block:: python

   SaveCSV(InputWorkspace='2999Clone', Filename='Cloner.csv')

- Compare the contents of `Clone.csv` and `Cloner.csv`, they should be the same
