.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

.. warning::

   This algorithm is being developed for a specific instrument. It might get changed or even 
   removed without a notification, should instrument scientists decide to do so.

The pattern is specified by providing a list of dSpacing values where Bragg peaks are expected. The
algorithm then fits peaks in those areas, emitting a dSpacing to TOF conversion coefficients for 
every peak. Those dSpacing to TOF relationships are then fitted to a linear function.

Usage
-----

**Example - Fitting two peaks:**

.. testcode:: ExTwoPeaks


Output:

.. testoutput:: ExTwoPeaks


.. categories::
