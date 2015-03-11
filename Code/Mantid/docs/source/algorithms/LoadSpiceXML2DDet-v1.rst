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

.. testcode:: ExLoadHB3AXMLData

  # Load data by LoadSpiceXML2DDet()
  LoadSpiceXML2DDet(Filename='HB3A_exp355_scan0001_0522.xml', 
      OutputWorkspace='s0001_0522', DetectorGeometry='256,256')    

  # Access output workspace and print out some result
  ws = mtd["s0001_0522"]

  print "Number of spectrum = %d." % (ws.getNumberHistograms())
  for i, j in [(0, 0), (255, 255), (136, 140), (143, 140)]:
      print "Y[%-3d, %-3d] = %.5f" % (i, j, ws.readY(i)[j])

.. testcleanup:: ExLoadHB3AXMLData

  ws = mtd["s0001_0522"]
  DeleteWorkspace(Workspace=str(ws))

Output:

.. testoutput:: ExLoadHB3AXMLData

  Number of spectrum = 256.
  Y[0  , 0  ] = 0.00000
  Y[255, 255] = 0.00000
  Y[136, 140] = 1.00000
  Y[143, 140] = 2.00000

.. categories::
