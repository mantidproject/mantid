.. _TrainingPythonAndMantid:

==================
 Python and Mantid
==================

Much of Mantid can be scripted using `Python <https://www.python.org>`_. There are currently three ways of running Mantid related Python code:

1. from MantidPlot: Python window (View->Script Window)

   * For writing and executing long multi-line scripts
   * Allows partial execution (Execute->Execute selection)

2. from MantidPlot: Script Interpreter (View->Toggle Script Interperter)

   * For executing single-line commands

3. from outside MantidPlot: run like any regular Python script

   * Python environment needs to be set up correctly either through :literal:`PYTHONPATH` environment variable, or explicitly in the script. For Linux and Mac the correct paths are usually set automatically, for Windows the simplest way to do this is to use the :literal:`mantidpython.bat` script that ships with Mantid and can be found in the :literal:`bin` directory of the installation. Running this will start a basic IPython session with all of the appropriate paths set up.
   * Mantid modules need to be imported explicitly in the script
   * MantidPlot functionality is not available (plots, instrument view, etc.)


Running algorithms with Python
##############################

Using algorithms in Python is like calling ordinary Python functions. The input properties of an algorithm become the function's arguments and the output properties the return values.

MantidPlot imports all algorithms for you but if you run a script from a command-line Python interpreter you will require:

.. code-block:: python

   from mantid.simpleapi import *

at the top of your script to access the functions.


Example: LoadAndMerge
#####################

The first step when working with Mantid is usually to load some data. This can be done using the general-purpose :ref:`algm-LoadAndMerge` algorithm:

.. code-block:: python

   run = LoadAndMerge('ILL/D17/317370.nxs')

The loaded workspace is assigned to the :literal:`run` variable and appears under the same name in the workspace list.


Positional and named arguments
##############################

As other Python functions, Mantid algorithms can be called using so called *positional arguments*:

.. code-block:: python

   converted = ConvertUnits(run, 'TOF', 'Elastic')

The order of the arguments corresponds to the order of the input properties in algorithms' documentation.

Python allows you to explicitly state which value is assigned to which argument. These are called *named arguments*:

.. code-block:: python

   converted = ConvertUnits(InputWorkspace=run, Target='TOF', EMode='Elastic')

This style is preferred since it improves code readability.


Return types
############

If there is a single output property, it is returned as is. However, if there are multiple output properties, a *named tuple* is returned:

.. code-block:: python

   # This algorithm has two output properties: OutputWorkspace and NumberOfFailures
   result = MedianDetectorTest(run)
   # Accessing the outputs in the named tuple:
   histogramCount = result.OutputWorkspace.getNumberHistograms()
   failedDetectorCount = result.NumberOfFailures
   print('Number of bad detectors: {}'.format(failedDetectorCount))


Output workspaces
#################

Almost all Mantid algorithms have an *OutputWorkspace* property. In the examples above this workspace was assigned to a Python variable directly. It is possible to add output workspaces to the workspace list without assigning them to variables:

.. code-block:: python

   # Explicitly use the name parameter
   LoadAndMerge('ILL/D17/317370.nxs', OutputWorkspace='run')

If an output workspace property is marked as *Optional*, you may have to use the above syntax to make the algorithm to generate the output.


Accessing items in the workspace list
#####################################

The workspace list in MantidPlot holds all workspaces in :ref:`Analysis Data Service <Analysis Data Service>`. The workspaces can be assigned to Python variables:

.. code-block:: python

   run = mtd['317370']

It is possible to import all workspaces at once:

.. code-block:: python

   mtd.importAll()
   # If there were workspaces named 'run' and 'reference'
   normalized = Divide(run, reference)


Accessing data in workspaces
############################

The actual data within the workspaces, if they are :ref:`MatrixWorkspaces <MatrixWorkspace>`, can be accessed as `numpy <http://www.numpy.org>`_ arrays in Python. Read only access:

.. code-block:: python

   # 'run' is assigned to a MatrixWorkspace
   # We want to access the first histogram
   workspaceIndex = 0
   xs = run.readX(workspaceIndex)  # Bin edges or 'points'
   ys = run.readY(workspaceIndex)  # Counts etc.
   es = run.readE(workspaceIndex)  # Errors.
   # xs, ys and es are 1D numpy arrays.
   first = xs[0]
   last = xs[-1]
   print('X range: {}'.format(last - first))

If mutable access is desired instead, you can use writable wrappers:

.. code-block:: python

   import math
   xs = run.dataX(23)
   ys = run.dataY(23)
   es = run.dataE(23)
   # Scale all values
   ys *= 0.99
   es *= 0.99
   # Set individual
   ys[13] = 2018.
   es[13] = math.sqrt(2018.)

The entire X, Y and E data can be accessed as 2D numpy arrays:

.. code-block:: python

   xs = run.extractX()
   ys = run.extractY()
   es = run.extractE()
   print(ys.ndim)
   print(xs.shape)
   print(ys.shape)

Access to metadata such as sample logs is possible:

.. code-block:: python

   # Read-only
   logs = run.run()
   print(logs.getProperty('sample.temperature').value)
   # For writable access, use mutableRun
   logs = run.mutableRun()
   replace = True
   logs.addProperty('sample.pressure', 23., replace)


Replaying workspace history
###########################

Every Mantid workspace has a history associated with it. The history stores all of the information regarding the creation of the specified workspace, i.e. algorithms and their properties. This information can be used to generate a Python script to rerun the algorithms that created that workspace.

To view the history of a workspace in MantidPlot, right click on the entry withing the workspace list and select Show History. A window detailing the history will appear.

.. figure:: /images/Training/UsingMantidFromPython/AlgorithmHistory.png
   :align: center

The two buttons, Script to File and Script to Clipboard allow you to save the corresponding Python script to a file or to the operating system's clipboard from where you can paste it to, for example, MantidPlot's Script Window. The resulting script from the above history would look like this:

.. code-block:: python

   LoadILLReflectometry(Filename='/net4/serdon/illdata/171/figaro/internalUse/rawdata/592724.nxs', OutputWorkspace='592724', XUnit='TimeOfFlight')
   GravityCorrection(InputWorkspace='592724', OutputWorkspace='592724_gc', FirstSlitName='slit3')
   Logarithm(InputWorkspace='592724_gc', OutputWorkspace='592724_gc')
   ConvertUnits(InputWorkspace='592724_gc', OutputWorkspace='592724_gc', Target='Wavelength', ConvertFromPointData=False)


Extending Mantid with new algorithms
####################################

It is possible to add new algorithms to Mantid using Python. Those who are interested should check the `Extending Mantid With Python <http://www.mantidproject.org/Extending_Mantid_With_Python>`_ tutorial.
