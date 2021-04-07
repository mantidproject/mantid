.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Normalises a workspace according to the good proton charge figure taken
from the Input Workspace log data, which is stored in the workspace's
:py:obj:`run object <mantid.api.Run>`. Every data point
(and its error) is divided by that number.
The good proton charge value is added to the normalized workspace
as the value of *NormalizationFactor* log.


ISIS Calculation Details
------------------------

The good proton charge **gd\_prtn\_chrg** is an summed value that
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

Usage
-----

**Example - Normalise by Current simple workspace**

.. testcode:: exNormaliseByCurrentSimple

   # Create two workspaces
   ws = CreateWorkspace(DataX=range(0,3), DataY=(17,12))

   # Add Good Proton Charge Log
   AddSampleLog(Workspace=ws, LogName='gd_prtn_chrg', LogText='10.0', LogType='Number')

   # Fetch the generated logs
   run1 = ws.getRun()
   log_p = run1.getLogData('gd_prtn_chrg')

   # Print the log value
   print("Good Proton Charge = {}".format(log_p.value))

   #Run the Algorithm
   wsN = NormaliseByCurrent(ws)
   norm_factor = wsN.getRun().getLogData('NormalizationFactor').value

   #Print results
   print("Before normalisation {}".format(ws.readY(0)))
   print("After normalisation  {}".format(wsN.readY(0)))
   print("Normalisation factor {}".format(norm_factor))


Output:

.. testoutput:: exNormaliseByCurrentSimple

   Good Proton Charge = 10.0
   Before normalisation [ 17.  12.]
   After normalisation  [ 1.7  1.2]
   Normalisation factor 10.0

.. categories::

.. sourcelink::
