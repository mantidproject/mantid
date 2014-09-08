.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Some Nexus files contain an instrument definition. This algorithm loads
the instrument from this definition. You may need to tell this algorithm
where in the Nexus file to find the Instrument folder, which contains
the instrument definition. It also looks to see if it contains a separate 
instrument parameter map. If yes this is loaded. If no, the algorithm will
attempt to load on paramter file on your disk from your instrument folder
with the name INST_Parameters.xml. Notification are displayed to information
what the algorithm does.

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
   inst1 = ws_1.getInstrument()
   comp1 = inst1.getComponentByName("the rings")
   print "Modified component name =", comp1.getName()

   # This workspace had no IDF loaded into it, so still has component named to "both rings".
   inst2 = ws_2.getInstrument()
   comp2 = inst2.getComponentByName("both rings")
   print "Unmodified component name =", comp2.getName()
   
Output:

.. testoutput:: ExLoadIDFFromnNexusSimple

   Modified component name = the rings
   Unmodified component name = both rings

.. categories::
