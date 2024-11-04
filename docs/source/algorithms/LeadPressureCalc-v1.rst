.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm uses Equations of state for Lead (111) to calculate the pressure of a given sample. The required inputs are the dSpacing (A) of the Lead (111) Bragg peak and the reference temperature (K).

Optionally, a further input of Target pressure (GPa) can be given, which will find the corresponding dSpacing for the pressure at the provided reference temperature.

Usage
-----------

**Example:**

.. code-block:: python

	from mantid.simpleapi import LeadPressureCalc

	LeadPressureCalc(dSpacing=2.85, T=312, TargetPressure=0.0043)

References
----------

The Equations of State and fit parameters used in this calculation can be found in:

Fortes, A. D. (2019). A revised equation of state for in situ pressure determination
using fcc-Pb (0 < P < 13 GPa, T > 100 K)

RAL Technical Report, RAL-TR-2019-002
# https://epubs.stfc.ac.uk/work/40740875

.. categories::

.. sourcelink::
