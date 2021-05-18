.. _01_parameters:

==========
Parameters
==========

We define a parameter as one of a set of variables that form the parameter
space over which the optimisation takes place, i.e. can be varied by the
fitting engine. Parameters are of a single type: Python float (C++ double).
Parameters must be declared in an ``init`` section so that the fitting
framework can find out what they are, i.e. you see them in the GUI.

To declare a parameter use the ``declareParameter()`` method that takes a
name & and an optional default value:

.. code-block:: python

    from mantid.api import *

    # Remember to choose either IFunction1D or IPeakFunction
    class Example1DFunction(IFunction1D): # or IPeakFunction

        def init(self):
            # Parameter with default value 1.0
            self.declareParameter("Constant", 1.0)

If not supplied the default value is ``0.0``
