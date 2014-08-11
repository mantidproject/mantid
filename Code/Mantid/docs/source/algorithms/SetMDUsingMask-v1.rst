.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm is used to replace values in a
:ref:`MDHistoWorkspace <MDHistoWorkspace>` but only at particular points.

A mask MDHistoWorkspace is provided, where non-zero values indicate
'true'. At these points, the corresponding value in the ValueWorkspace
will be set. Any 'false' points in the MaskWorkspace are skipped.

If ValueWorkspace is not specified, the you must specify Value, which is
a a simple number to set.

In matlab, the equivalent function call would be WS[mask] =
OtherWS[mask]

See `this page on boolean
operations <MDHistoWorkspace#Boolean_Operations>`__ for examples of how
to create a mask.

Usage (Python)
--------------

| ``# This will zero-out any values below the threshold of 123``
| ``MaskWS = WS < 123``
| ``ModifiedWS = SetMDUsingMask(InputWorkspace=WS, Value="0", MaskWorkspace=MaskWS)``

.. categories::
