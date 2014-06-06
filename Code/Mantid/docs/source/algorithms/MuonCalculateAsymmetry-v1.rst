.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Converts loaded/prepared Muon data to a data suitable for analysis.

Supports three modes:

-  PairAsymmetry - asymmetry is calculated for a given pair of groups,
   using the alpha value provided.
-  GroupAsymmetry - asymmetry between given group and Muon exponential
   decay is calculated.
-  GroupCount - **no asymmetry is calculated**, pure counts of the
   specified group are used.

For every mode, either one or two data acquisition period workspaces can
be provided. PeriodOperation determines in which way period data will be
merged at the end.

.. categories::
