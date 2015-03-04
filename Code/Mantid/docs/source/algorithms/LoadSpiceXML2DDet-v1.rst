.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm is to import SPICE-generated XML file that
records data of one measurement by a (two-dimensional) Anger camera
and create a MatrixWorkspace to contain the detectors' counts, monitor counts 
and other sample log data.


Format of SPICE XML data file
#########################

The SPICE XML data file contains four sections
 - ???
 - ???
 - Parameters
 - Counts

Each of them contains child nodes for detailed information, such as ... 
 


Output Worskpaces
#################

One MatrixWorskpaces will be exported from the algorith. 

'OutputWorkspace' is ... 



Usage
-----

**Example - load a HB3A SPICE .xml file:**

.. testcode:: ExLoadHB3AMXLData


.. testcleanup:: ExLoadHB3AXMLData

  #DeleteWorkspace(infows)

Output:

.. testoutput:: ExLoadHB3AXMLData


.. categories::
