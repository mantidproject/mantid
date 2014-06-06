.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Compare two MDWorkspaces (`MDEventWorkspace <MDEventWorkspace>`__ or
`MDHistoWorkspace <MDHistoWorkspace>`__) to see if they are the same.
This is mostly meant for testing/debugging use by developers.

**What is compared**: The dimensions, as well as the signal and error
for each bin of each workspace will be compared.

**`MDEventWorkspace <MDEventWorkspace>`__**: the events in each box will
be compared if the *CheckEvents* option is checked. The events would
need to be in the same order to match.

.. categories::
