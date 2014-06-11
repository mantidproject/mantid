.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm loads an ARIEL-style 5-column ASCII .cal file into up to
3 workspaces: a GroupingWorkspace, OffsetsWorkspace and/or MaskWorkspace.

The format is

-  Number: ignored.\* UDET: detector ID.\* Offset: calibration offset.
   Goes to the OffsetsWorkspace.
-  Select: 1 if selected (not masked out). Goes to the MaskWorkspace.
-  Group: group number. Goes to the GroupingWorkspace.

.. categories::
