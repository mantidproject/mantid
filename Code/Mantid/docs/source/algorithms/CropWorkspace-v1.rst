.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Extracts a 'block' from a workspace and places it in a new workspace
(or, to look at it another way, lops bins or spectra off a workspace).

CropWorkspace works on workspaces with common X arrays/bin boundaries or
on so-called `ragged workspaces <Ragged Workspace>`__. If the input
workspace has common bin boundaries/X values then cropping in X will
lead to an output workspace with fewer bins than the input one. If the
boundaries are not common then the output workspace will have the same
number of bins as the input one, but with data values outside the X
range given set to zero.

If an X value given for XMin/XMax does not match a bin boundary (within
a small tolerance to account for rounding errors), then the closest bin
boundary within the range will be used. Note that if none of the
optional properties are given, then the output workspace will be a copy
of the input one.

.. categories::
