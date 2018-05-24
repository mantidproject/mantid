.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This workflow algorithm appends new data to an existing multidimensional workspace. It allows the accumulation of data in a single MDWorkspace as you go, e.g. during an experiment.

Using the FileBackEnd and Filename properties the algorithm can produce a file-backed workspace.
Note that this will significantly increase the execution time of the algorithm.

Input properties which are not described here are identical to those in the :ref:`algm-CreateMD` algorithm.

InputWorkspace
##############
The MDEventWorkspace to append data to.

DataSources
###########
These can be workspace names, file names or full file paths. Not all of the data need to exist when the algorithm is called. If data are named which have previously been appended to the workspace they will not be appended again. Note that data are known by name, it is therefore possible to append the same data again if the data source is renamed.

Clean
###########
It is possible to get confused about what data has been included in an MDWorkspace if it is built up slowly over an experiment. Use this option to start afresh; it creates a new workspace using all of the data in DataSources which are available, rather then appending to the existing workspace.

Workflow
########

.. diagram:: AccumulateMD-v1_wkflw.dot

Usage
-----

**Simple Example**

.. testcode:: ExSimpleAccumulate

    # Create some sample data
    sample_data_1 = CreateSimulationWorkspace(Instrument='MAR', BinParams=[-3,1,3], UnitX='DeltaE')
    AddSampleLog(Workspace=sample_data_1,LogName='Ei',LogText='3.0',LogType='Number')
    sample_data_2 = CreateSimulationWorkspace(Instrument='MAR', BinParams=[-3,1,3], UnitX='DeltaE')
    AddSampleLog(Workspace=sample_data_2,LogName='Ei',LogText='3.0',LogType='Number')

    # Create an MDWorkspace withdata from sample_data_1
    md_ws = CreateMD(sample_data_1, Emode='Direct', Alatt=[1.4165, 1.4165,1.4165], Angdeg=[90, 90, 90], u=[1, 0, 0,], v=[0,1,0])

    # Append data from sample_data_2 to the existing workspace
    # Note: sample_data_1 will not be appended as it is already in the workspace
    #       sample_data_3 will not be appended as it does not exist
    acc_ws = AccumulateMD(md_ws, 'sample_data_1,sample_data_2,sample_data_3', Alatt=[1.4165, 1.4165, 1.4165], Angdeg=[90, 90, 90], u=[1, 0, 0,], v=[0,1,0])

    # acc_ws should have double the number of events that md_ws has
    print("There are {} events in each of the two data workspaces.".format(md_ws.getNEvents()))
    print("The accumulated data workspace contains {} events.".format(acc_ws.getNEvents()))
  
Output:

.. testoutput:: ExSimpleAccumulate

    There are 5508 events in each of the two data workspaces.
    The accumulated data workspace contains 11016 events.


.. categories::

.. sourcelink::
