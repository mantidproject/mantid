.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Retrieves the algorithm history of the workspace and saves it to an
IPython notebook file or Python variable.

A time range can be specified which will restrict the algorithms in
the notebook to those which were executed between the given times,
if no end time was specified then algorithms from the start time up
to the current time will be included in the generated notebook.

Start and end times are given in ISO8601 format: YYYY-MM-DD HH:mm:ss,
for example 3:25 PM on July the 4th 2014 would be 2014-07-04 15:25:00.

Usage
-----

.. The examples for this algorithm do not show output as the output 
   is a large quantity of JSON. The algorithm is tested by unit tests.

**Example - generate an IPython notebook for a workspace:**

.. code-block:: python

    #create a workspace and run some operations on it
    ws = CreateSampleWorkspace()
    ws = CropWorkspace(ws, XMin=7828.162291, XMax=11980.906921)
    ws = Power(ws, Exponent=1.5)
    ws = RenameWorkspace(ws, OutputWorkspace="MyTestWorkspace")

    notebook_text = GenerateIPythonNotebook(ws)
    print notebook_text.strip()

**Example - generate a python notebook giving a range of start times:**

.. code-block:: python

    import time

    # Do some operations on the workspace with a pause between them
    ws = CreateSampleWorkspace()
    ws = CropWorkspace(ws, XMin=7828.162291, XMax=11980.906921)
    time.sleep(2)
    ws = Power(ws, Exponent=1.5)
    ws = RenameWorkspace(ws, OutputWorkspace="MyTestWorkspace")

    # Get the execution time of the last algorithm and subtract 1 second
    history = mtd['MyTestWorkspace'].getHistory()
    last = history.getAlgorithmHistory(history.size() - 1)
    from_time = last.executionDate() - int(1e9)

    # Generate a notebook with a given start time
    notebook_text = GenerateIPythonNotebook(ws, StartTimestamp=str(from_time))
    print notebook_text.strip()

**Example - generate a python notebook and save it to file:**

.. code-block:: python

    import os
    
    #create a workspace and run some operations on it
    ws = CreateSampleWorkspace()
    ws = CropWorkspace(ws, XMin=7828.162291, XMax=11980.906921)
    ws = Power(ws, Exponent=1.5)
    ws = RenameWorkspace(ws, OutputWorkspace="MyTestWorkspace")

    path = os.path.join(os.path.expanduser("~"), 'mynotebook.ipynb')
    GenerateIPythonNotebook(ws, Filename=path)

    with open (path, 'r') as notebook:
      print notebook.read().strip()

.. categories::

.. sourcelink::
