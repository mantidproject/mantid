.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm looks for an internally-stored monitor workspace on the input workspace and
sets it as the output workspace if found. The input workspace will no longer hold a reference
to the monitor workspace after this algorithm has been run on it.
If no monitor workspace is present the algorithm will fail.

This does not extract monitor data that is stored with the detector data in the workspace.

Usage
-----

**Example: Extracting an internally stored monitor workspace**

.. This is encoded as a non tested block as it is currently not possible to create a workspace with an internal monitor workspace in python.

.. code-block:: python

   #If no monitor workspace is present the algorithm will fail.
   wsMonitor = ExtractMonitorWorkspace(ws)

.. categories::

.. sourcelink::
