.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The algorithm will copy the sample logs in the input workspace to the
the output workspace using one of three merge strategies.

-  MergeReplaceExisting: Default option. Copy logs from the input
   workspace to the output workspace and replace any existing logs with
   the same name.
-  MergeKeepExisting: Keep the existing logs in the output workspace and
   don't modify them, but append any new ones from the input workspace.

Note that this will not concatenate values or ranges. The algorithm will
choose to keep the value of any the log already present in the output
workspace, leaving it untouched.

-  WipeExisting: Dump any logs that are in the output workspace and
   replace them with the logs from the input workspace.

.. categories::
