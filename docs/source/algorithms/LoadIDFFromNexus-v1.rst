.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Some Nexus files contain an instrument definition. This algorithm loads
the instrument from this definition. You may need to tell this algorithm
where in the Nexus file to find the Instrument folder that contains
the instrument definition.

It also looks to see if it contains a separate instrument parameter map.
If yes this is loaded. If no, the algorithm will
attempt to load on parameter file on your disk from your instrument folder
with the name INST_Parameters.xml.
This may be overridden by a parameter correction file, which can be
used to correct out of date embedded parameters.

A parameter correction file contains a list of parameter files,
each with a non-overlapping date range and an append flag.
If a parameter correction file is found,
its list is compared to the workspace's run start date.
If this date occurs within one of the date ranges, the file with that date range
is used as the parameter file.
This parameter file must be in the same directory as the correction file.
If the append flag is true this parameter file is used in addition to any other parameters
that would be used, else it replaces the those parameters.

Notifications are displayed to inform, what the algorithm is doing.

Usage
-----
**Example - Load an IDF with differently named component**

.. include:: ../usagedata-note.txt

.. testcode:: ExLoadIDFFromnNexusSimple

   result = Load("MUSR00015189")
   group = result[0]
   ws_1 = group[0]
   ws_2 = group[1]

   # musr_with_namechange.nxs has component named 'the rings' instead of 'both rings')
   LoadIDFFromNexus(ws_1, "musr_with_namechange.nxs","/mantid_workspace_1")

   # This workspace had the IDF loaded into it, so getting component renamed to "the rings".
   component_info_1 = ws_1.componentInfo()
   index_1 = component_info_1.indexOfAny("the rings")
   print("Modified component name = {}".format(component_info_1.name(index_1)))

   # This workspace had no IDF loaded into it, so still has component named to "both rings".
   component_info_2 = ws_2.componentInfo()
   index_2 = component_info_2.indexOfAny("both rings")
   print("Unmodified component name = {}".format(component_info_2.name(index_2)))

Output:

.. testoutput:: ExLoadIDFFromnNexusSimple

   Modified component name = the rings
   Unmodified component name = both rings

.. categories::

.. sourcelink::
