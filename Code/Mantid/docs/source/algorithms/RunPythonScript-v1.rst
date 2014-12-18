.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Algorithm that will run a snippet of python code. This is meant to be
used by :ref:`algm-LoadLiveData` to perform some processing.

The input & output workspaces can be accessed from the Python code using
the variable names 'input' & 'output' respectively.

Within your code "input" is an actual reference to the input workspace, while "output" is a string of the output workspace name.
You are expected to create the output workspace yourself.

Usage
-----

**Example - Summing all the counts in a workspace**

.. testcode:: exRunPythonScript

    ws = CreateSampleWorkspace("Histogram","Multiple Peaks")
    
    script = """
    logger.notice('Logging from within the python script!')
    wsOut = Integration(input,OutputWorkspace=output)
    wsOut = SumSpectra(wsOut)
    """

    RunPythonScript(script,ws,"wsOut")

    wsOut = mtd["wsOut"]
    print "The workspace contained a total of %i counts" % wsOut.readY(0)[0]

Output:

.. testoutput:: exRunPythonScript

    The workspace contained a total of 9599 counts


.. categories::
