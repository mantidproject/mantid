.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm saves an ARIEL-style 5-column ASCII .cal file.

The format is

-  Number: ignored.\* UDET: detector ID.\* Offset: calibration offset.
   Comes from the OffsetsWorkspace, or 0.0 if none is given.
-  Select: 1 if selected (not masked out). Comes from the MaskWorkspace,
   or 1 if none is given.
-  Group: group number. Comes from the GroupingWorkspace, or 1 if none
   is given.

.. categories::
