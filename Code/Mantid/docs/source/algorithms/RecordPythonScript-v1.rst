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


.. testcode:: RecordPythonScript
    
    from threading import Thread
    import os, time

    #find a suitable directory to save the file
    fileDir = config["defaultsave.directory"]
    if not os.path.isdir(fileDir):
        #use the users home directory if default save is not set
        fileDir = os.path.expanduser('~')
    outputFile = os.path.join(fileDir,"MyRecording.py")

    def startRecording():
        try:
            RecordPythonScript(outputFile)
        except RuntimeError:
            pass
    thread = Thread(target = startRecording)
    thread.start()

    # a short pause to allow the thread to start
    time.sleep(0.1)

    ws = CreateSampleWorkspace("Event","Multiple Peaks")
    wsOut=CreateFlatEventWorkspace(ws,RangeStart=15000,RangeEnd=18000)
    wsOut=RebinToWorkspace(wsOut,ws,PreserveEvents=True)

    # This will cancel the rocording algorithm
    # you can do the same in the GUI 
    # by clicking on the details button on the bottom right
    AlgorithmManager.newestInstanceOf("RecordPythonScript").cancel()
    thread.join()

    #Load and print the resulting file
    print "The result file has the following python recorded"
    with open(outputFile, "r") as file:
        print file.read().rstrip()

    #cleanup
    os.remove(outputFile)

Output:

.. testoutput:: RecordPythonScript
    :options: +NORMALIZE_WHITESPACE

    The result file has the following python recorded
    CreateSampleWorkspace(OutputWorkspace='ws',WorkspaceType='Event',Function='Multiple Peaks',UserDefinedFunction='',NumBanks='2',BankPixelWidth='10',NumEvents='1000',Random='0',XUnit='TOF',XMin='0',XMax='20000',BinWidth='200',PixelSpacing='0.0080000000000000002',BankDistanceFromSample='5')
    CreateFlatEventWorkspace(InputWorkspace='ws',RangeStart='15000',RangeEnd='18000',OutputWorkspace='wsOut')
    RebinToWorkspace(WorkspaceToRebin='wsOut',WorkspaceToMatch='ws',OutputWorkspace='wsOut',PreserveEvents='1')




.. categories::
