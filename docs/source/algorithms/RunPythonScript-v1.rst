.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Algorithm that will run a snippet of python code. This is meant to be
used by :ref:`algm-LoadLiveData` to perform some processing.

The input & output workspaces can be accessed from the Python code
using the variable names ``input`` and ``output`` respectively.

Within your code ``input`` is an actual reference to the input
workspace, while ``output`` is a string of the output workspace name.
You are expected to create the output workspace yourself.

Because of boilerplate code added to the python script being run,
stack-traces for errors will be five (5) lines greater than where the
error occured in the supplied python code.

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

    RunPythonScript(InputWorkspace=ws,Code=script,OutputWorkspace="wsOut")

    wsOut = mtd["wsOut"]
    print("The workspace contained a total of {:d} counts".format(int(wsOut.readY(0)[0])))

Output:

.. testoutput:: exRunPythonScript

    The workspace contained a total of 9599 counts

The same script could be saved to a file and specified using the
``Filename`` property instead of the ``Code`` property.

.. categories::

.. sourcelink::
