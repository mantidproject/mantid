.. _RectangularDetector:

Rectangular Detector
====================

The rectangular detector is a detector bank that is marked as being a regular rectangular bank. It is a particular class in Mantid (RectangularDetector).

In order to be a RectangularDetector, a bank has to have these characteristics:

- All pixels are the same size and shape (can be any size/shape)
- The pixels are regularly spaced (the X/Y spacing can be different)
- The pixels form a rectangular array (there can be gaps between pixels).

Several instruments including TOPAZ, PG3 and SNAP instruments use RectangularDetectors.

See the :ref:`InstrumentDefinitionFile <InstrumentDefinitionFile>` page for instructions on defining a rectangular detector.


.. categories:: Concepts
