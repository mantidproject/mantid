.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Normalises a workspace according to the good proton charge figure taken
from the Input Workspace log data, which is stored in the workspace's
`Sample <http://www.mantidproject.org/Sample>`_ object). Every data point (and its error) is divided
by that number.

ISIS Calculation Details
------------------------

The good proton charge **gd\_ptrn\_chrg** is an summed value that
applies across all periods. It is therefore suitable to run
NormaliseByProtonCharge for single-period workspaces, but gives
incorrect normalisation for multi-period workspaces. If the algorithm
detects the presences of a multi-period workspace, it calculates the
normalisation slightly differently. It uses the **current\_period** log
property to index into the **proton\_charge\_by\_period** log data array
property.

EventWorkspaces
###############

If the input workspace is an :ref:`EventWorkspace <EventWorkspace>`, then
the output will be as well. Weighted events are used to scale by the
current (see the :ref:`algm-Divide` algorithm, which is a child
algorithm being used).

.. categories::
