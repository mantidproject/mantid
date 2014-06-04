.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

A `GroupingWorkspace <GroupingWorkspace>`__ is a simple workspace with
one value per detector pixel; this value corresponds to the group number
that will be used when focussing or summing another workspace.

This algorithm creates a blank GroupingWorkspace. It uses the
InputWorkspace, InstrumentName, OR InstrumentFilename parameterto
determine which Instrument to create.

If the OldCalFilename parameter is given, the .cal ASCII file will be
loaded to fill the group data.

If the GroupNames parameter is given, the names of banks matching the
comma-separated strings in the parameter will be used to sequentially
number the groups in the output.

.. categories::
