.. _04_exercise_5:

==========
Exercise 5
==========

The aim of this exercise is to implement a simple linear fitting function
defined as:

.. math::

    \LARGE y = A_0 + A_1x

The function should have 2 parameters:

#. ``A0``: intercept with the y-axis.
#. ``A1``: gradient of the function.

Define a class called ``PyLinearFunction``

* Add an ``init`` function that declares the two parameters.
* Add a ``function1D`` function that returns the y values for the given x values
  as a numpy array.
* Add the appropriate line to register it with Mantid.

To test:

#. Execute your code in the script window to register the function with Mantid.
#. Load file *GEM38370_Focussed_Legacy.nxs*.
#. Plot spectrum number 5.
#. Activate the fitting tool.
#. Right click on the plot and select "Add Other Function...".
#. Select ``PyLinearFunction``.
#. Change ``StartX`` to 0.1 and ``EndX`` to 2.0
#. Click Fit!

Once finished check your answer with the provided :ref:`05_emwp_sol`
