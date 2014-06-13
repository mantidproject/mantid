.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

.. raw:: mediawiki

   {{BinaryOperation|verb=added|prep=to|symbol=<math>+</math>}}

EventWorkspace note
###################

For `EventWorkspaces <EventWorkspace>`__, the event lists at each
workspace index are concatenated to create the output event list at the
same workspace index. Note that in some (rare:sup:`\*`) cases, these
event lists might be from different detectors; this is not checked
against and the event lists will be concatenated anyway. This may or may
not be your desired behavior. If you wish to merge different
EventWorkspaces while matching their detectors together, use the
:ref:`algm-MergeRuns` algorithm.

:sup:`\*` This could happen, for example, if the workspace operands have
not both been processed in an identical fashion and the detectors have
somehow been grouped differently.

.. categories::
