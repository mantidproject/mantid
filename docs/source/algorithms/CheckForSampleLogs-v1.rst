.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Check if the workspace has some given sample logs. An empty string will be returned if all the logs are found, otherwise the result is  an error message 


Usage
-----

.. include:: ../usagedata-note.txt

.. testcode:: CheckForSampleLogs

    #load a workspace with logs
    ws=Load("CNCS_7860")
    
    sampleLogs="Phase1,Phase3"
    result=CheckForSampleLogs(ws,sampleLogs)
    if len(result)==0:
        print("We found logs for {}".format(sampleLogs))
    sampleLogs="DJIA"
    result=CheckForSampleLogs(ws,sampleLogs)
    print(result.strip())

.. testcleanup:: CheckForSampleLogs

    DeleteWorkspace('ws')

Output:

.. testoutput:: CheckForSampleLogs

    We found logs for Phase1,Phase3
    Property DJIA not found

.. categories::

.. sourcelink::
