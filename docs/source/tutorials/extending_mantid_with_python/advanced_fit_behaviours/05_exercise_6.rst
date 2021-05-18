.. _05_exercise_6:

==========
Exercise 6
==========

The aim of this exercise is to implement a function to fit the output data
from :ref:`05_exercise_4`. For simplicity a solution file, *11001_deltaE.nxs*,
is provided with the training data.

The peak can be fairly well approximated using a Lorentz function:

.. math::

    \LARGE \frac{A}{π}(\frac{\frac{Γ}{2}}{(x-c)^2 + (\frac{Γ}{2})^2})

where ``A`` is the amplitude, ``Γ`` is the full width at half maximum and
``c`` is the peak centre. We will first define this as a simple 1D function
and then improve it to use the peak function capabilities.

Simple 1D
=========

* Define a new 1D function called ``Lorentz``.
* It should have 3 parameters corresponding to the parameters described above.
* Write the ``function1D`` method that evaluates the required values from the
  input x data using the definition as above. (Hint: you can use the python
  math module for
  `pi <https://docs.python.org/2/library/math.html#constants>`_).

Test this implementation:

#. Load the data file.
#. Plot the spectrum.
#. Use the fit browser (using the fit function tool button (looks like a peak
   with a vertical red line on top)).
#. Right click on plot and select "Add other function...".

* You'll want to see how the fit progresses so set the log level in the
  Messages Box to information by right clicking in the
  window and selecting Log Level->Information. This will display additional
  information as the fit proceeds.
* You may need to adjust the parameter initial values in the Fit Function window

Analytical Derivative
=====================

Extend the above 1D function and add an analytical derivative by adding a
``functionDeriv1D`` method. The derivatives w.r.t to each of the parameters
are as follows:

.. math::

    \LARGE A \longrightarrow \frac{2}{π}\frac{Γ}{Γ^2 + 4(x - c)^2}

    \LARGE c \longrightarrow \frac{A}{π}\frac{Γ(x - c)}{[\{\frac{Γ}{2}\}^2 + (x - c)^2]^2}

    \LARGE Γ \longrightarrow - \frac{2A}{π}\frac{Γ^2 - 4(x - c)^2}{[Γ^2 + 4(x - c)^2]^2}

Re-run the fit using the above steps.

Peak Function
=============

* Make a copy of the Lorentz function and rename it to LorentzPeak.
* Make this class an ``IPeakFunction`` instead of ``IFunction1D`` and change
  the methods from ``function1D`` to ``functionLocal`` and ``functionDeriv1D``
  to ``functionDerivLocal``.
* Add the :ref:`required methods <04_peak_function_methods>`
  for Mantid to interact with this as a peak function.
* Retest using the steps above with the exception that when you right click on
  the plot choose the "Add peak..." menu rather than "Add other function...".
* You should now have interactivity in the GUI where you can set the initial
  values using the tools and get a quicker fit.

Once finished check your answer with the provided :ref:`06_emwp_sol`
