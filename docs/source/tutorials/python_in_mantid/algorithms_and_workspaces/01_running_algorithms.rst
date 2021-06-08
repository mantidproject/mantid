.. _01_running_algorithms:

==============================
Running Algorithms With Python
==============================


Each of the algorithms that are available in Mantid has a corresponding Python function. The function has the same name as the algorithm and is case sensitive, so the case must match when calling from Python.

Whether you run a script in Workbench or from a command-line Python interpreter you are required to include:

.. code-block:: python

    from mantid.simpleapi import *

at the top of your script to access the functions.

You will also need to ensure that your Python path is set correctly to make the Mantid package available. For Linux and Mac, the correct paths are set automatically, for Windows the simplest way to do this is to use the `mantidpython.bat` script that ships with Mantid and can be found in the bin directory of the installation. Running this will start a basic IPython session with all of the appropriate paths set up.


A Simple Example
================

The simplest exercise is to load a data file. This is done using the :ref:`algm-Load` function, i.e.

.. code-block:: python

    from mantid.simpleapi import *
    # This example just has .RAW extension but it is able to load all
    # file types that Mantid is aware of.
    # run = Load('filename.nxs')
    run = Load('HRP39182.RAW')

The workspace that was the result of the load is returned and assigned to the `run` variable. The function is defined such that the created workspace will have the name `run`, which will appear in the workspace list.
This feature is common to all of the single-shot functions: "The output workspace name is picked up from the variable that the function return is assigned to."

This workspace can then be carried into another algorithm, :ref:`algm-ConvertUnits` for example. Running

.. code-block:: python

    run = ConvertUnits(InputWorkspace=run, Target='dSpacing')

will execute the :ref:`algm-ConvertUnits` algorithm with the input workspace set as `run` and the target unit set to `dSpacing`. As the function is assigned to the run variable, the output replaces the input workspace. If a different variable were used then the output would have ended up in workspace name that matches that variable name.


Keywords
========

All algorithms can be called using the arguments just given by position, i.e.

.. code-block:: python

    # All arguments provided as pure positional arguments:
    run = ConvertUnits(run, 'dSpacing', 'Direct', 85, False)

It is however advisable when writing scripts to use the keyword syntax that Python offers, i.e.

.. code-block:: python

    # All arguments provided with keywords:
    run = ConvertUnits(InputWorkspace=run, Target='dSpacing', EMode='Direct', EFixed=85, AlignBins=False)

This style is preferred as the author or another person attempts to read the script at a later date it is much easier to decipher the intent of the second style versus the first, especially with the last `True/False` parameter.

Return Types
============

Where an algorithm has more than one output property, the return value will arrive as a Python tuple.

.. code-block:: python

    ws = CreateSampleWorkspace()

    # The result variable will contain a tuple: (OutputWorkspace, JoinWavelength)
    outWS, wavelength = UnwrapMonitor(InputWorkspace=ws,LRef=11)

    print("OutputWorkspace is a: ")
    print(type(outWS))
    print("JoinWavelength is a: ")
    print(type(wavelength))

    # Alternatively we can unpack the tuple later
    result = UnwrapMonitor(InputWorkspace=ws,LRef=11)

    print("OutputWorkspace is a: ")
    print(type(result[0]))
    print("JoinWavelength is a: ")
    print(type(result[1]))

    # From Mantid 3.10 named tuples can be used
    print("OutputWorkspace is a: ")
    print(type(result.OutputWorkspace))
    print("JoinWavelength is a: ")
    print(type(result.JoinWavelength))

Failing to unpack a tuple, or calling workspace methods directly on the tuple object will result in error messages such as `AttributeError: 'tuple' object has no attribute 'getNumberHistograms()'`.
