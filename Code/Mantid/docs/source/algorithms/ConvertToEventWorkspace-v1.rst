.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm takes a :ref:`Workspace2D <Workspace2D>` 
with any binning or units as its input. An event is created for each 
bin of each histogram, except if the bin count is 0.0 (unless 
``GenerateZeros`` is true). Infinity and NAN (not-a-number) values 
are always skipped.

Each event is created with an X position (typically time-of-flight)
equal to the **center** of the bin. The weight and error of the
event are taken from the histogram value.

If the ``GenerateMultipleEvents`` option is set, then instead of a single
event per bin, a certain number of events evenly distributed along the X
bin are generated. The number of events generated in each bin is
calculated by N = (Y/E)^2. However, it is limited to a max of
``MaxEventsPerBin`` and a minimum of 1 event per bin.

Note that using ``GenerateZeros`` or ``GenerateMultipleEvents`` may use a
lot of memory!

.. categories::
