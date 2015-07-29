
.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm calculates an S(Q, w) from the fitted results of a ConvFit: 1 or
2 Lorentzians.  The Q, w range is that for the input _red workspace. A
resolution _res is also needed for the elastic/delta-function peak.

The object is to create a S(Q, w) with an energy range greater than that of the
measured Q, w to provide a better calculation for multiple scattering.

The ParameterFile workspace must contain the following sample logs:

- program="ConvFit"
- 'lorenztians'

Usage
-----

**Example - MuscatSofQW**

.. testcode:: exMuscatSofQW

    TODO

Output:

.. testoutput:: exMuscatSofQW

    TODO

.. categories::

.. sourcelink::

