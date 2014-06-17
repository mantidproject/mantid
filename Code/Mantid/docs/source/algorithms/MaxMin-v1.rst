.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The algorithm creates a new 2D workspace containing the first maxima
(minima) for each spectrum, as well as their X boundaries and error.
This is used in particular for single crystal as a quick way to find
strong peaks. By default, the algorithm returns the maxima.

The :ref:`algm-Max` and :ref:`algm-Min` algorithms are just calls to the
:ref:`algm-MaxMin` algorithm, with the ShowMin flag set to true/false
respectively.

.. categories::
