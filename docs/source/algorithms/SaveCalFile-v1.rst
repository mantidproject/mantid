.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm saves an
`ARIEL-style <http://www.isis.stfc.ac.uk/instruments/gem/software/ariel-installation-instructions6723.html>`__ 
5-column ASCII .cal file. Here is an excerpt::

  # Calibration file for instrument POWGEN written on 2014-02-14T18:46:31.610072000.
  # Format: number    UDET         offset    select    group
          0             -1      0.0000000       1       0
          1         110000      0.0001250       1       1
          2         110001      0.0003750       1       1
          3         110002     -0.0020000       1       1
          4         110003      0.0000000       0       1
          5         110004      0.0000000       0       1


The format is

- number: ignored.
- UDET: detector ID.
- offset: calibration offset used in :ref:`algm-AlignDetectors`. Comes from 
  the ``OffsetsWorkspace``, or 0.0 if none is given.
- select: 1 if selected (use the pixel). Comes from the ``MaskWorkspace``,
  or 1 if none is given.
- group: what group to focus to in :ref:`algm-DiffractionFocussing`. Comes from the 
  ``GroupingWorkspace``, or 1 if none is given.

:ref:`algm-LoadCalFile`

.. categories::

.. sourcelink::
