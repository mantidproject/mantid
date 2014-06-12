.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

PoldiMerge takes a list of workspace names and adds the counts,
resulting in a new workspace. The difference to Plus is that it performs
some POLDI-specific tests that determine whether merging those files is
sensible or not. The following requirements have to be fulfilled:

-  The time-binning (x-data) of all workspaces must match (offset as
   well as width of time bins)
-  These quantities from the sample log:

   -  Position of the sample table (x, y and z)
   -  Rotation speed of the chopper

The algorithm does not perform partial summation - if any of the
workspaces does not fulfill the criteria, the intermediate result is
discarded.

.. categories::
