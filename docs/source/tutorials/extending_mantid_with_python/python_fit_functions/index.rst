.. _emwp_py_fit_funcs:

====================
Python Fit Functions
====================

.. toctree::
   :hidden:
   :glob:
   :maxdepth: 1

   01_parameters
   02_simple_1d_functions
   03_attributes
   04_exercise_5

Mantid's optimisation framework is split into different components:

* Function evaluation.
* Cost function calculation.
* Minimization algorithms.

Each of these can be swapped out for a different component that has some
other behaviour to maximise flexibility. Only the functions themselves
can be written in Python. The new Python functions are treated on exactly the
same level as the shipped ``C++`` functions and offer the same level of
interactivity within the GUI.

Function Types
==============

Mantid currently has the concept of 2 different function types:

#. :ref:`mantid.api.IFunction1D` - A general 1D function defined over some set
   of x values. Does not require a derivative.
#. :ref:`mantid.api.IPeakFunction` - A function where the concept of a width,
   a height and a centre can be defined. Currently requires a derivative.

The basic class structure for a function definition looks like:

.. code-block:: python

    from mantid.api import *
    import numpy as np

    # You choose which type you would like by picking the super class
    class Example1DFunction(IFunction1D): # or IPeakFunction

        def category(self):
            return 'Examples'

       # explained later

    # Register with Mantid
    FunctionFactory.subscribe(Example1DFunction)

The ``category`` is optional and defines where the function is shown in some
parts of the GUI.


**Contents**

* :ref:`01_parameters`
* :ref:`02_simple_1d_functions`
* :ref:`03_attributes`
* :ref:`04_exercise_5`
