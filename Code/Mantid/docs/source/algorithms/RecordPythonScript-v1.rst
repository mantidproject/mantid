.. algorithm::

.. summary::

.. alias::

.. properties::

This algorithm records the python commans of any algoithms that execute while it is running and saves them out to a file.

This is similar to extracting the history of a workspace, but outputs evey algorithm that occurs during it's execution rather than just for a specific workspace.

The file is written out once this algotithm is cancelled, either using the running algorithm details button or via python.


Usage
-----

**Example:**

.. This example is written as code blocks as it would require multi threading to run singly
.. Which is too complicated for a usage example

.. code-block:: python
    
    RecordPythonScript("MyRecording.py")

    # Run some algorithms in another python tab or via the GUI
    # For example:
    # ws = CreateSampleWorkspace("Event","Multiple Peaks")
    # wsOut=CreateFlatEventWorkspace(ws,RangeStart=15000,RangeEnd=18000)
    # wsOut=RebinToWorkspace(wsOut,ws,PreserveEvents=True)

    # Then cancel the algorithm either using the GUI
    # or executing this in another python tab
    # AlgorithmManager.cancelAll()


Output:

.. code-block:: python

    #a file containing the following:
    CreateSampleWorkspace(OutputWorkspace='ws',WorkspaceType='Event',Function='Multiple Peaks',UserDefinedFunction='',NumBanks='2',BankPixelWidth='10',NumEvents='1000',Random='0',XUnit='TOF',XMin='0',XMax='20000',BinWidth='200')
    CreateFlatEventWorkspace(InputWorkspace='ws',RangeStart='15000',RangeEnd='18000',OutputWorkspace='wsOut')
    RebinToWorkspace(WorkspaceToRebin='wsOut',WorkspaceToMatch='ws',OutputWorkspace='wsOut',PreserveEvents='1')



.. categories::
