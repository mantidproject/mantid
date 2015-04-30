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
#############################

The SPICE XML data file contains four sections under parent node *SPICErack*.
Each section contains child nodes for detailed information.

 - Header: instrument name, reactor power, experiment title and number, scan number and etc.
 - Motor_Position: positions of motor *m1*, *marc*, *2theta*, *chi*, *phi*, *omega* and etc. 
 - Parameter_Positions: reading of sample environment devices, such as temperature at sample.  
 - Counters: counting time, monitor counts, and *N x N* detectors' counts,


Counts of 2D detector
+++++++++++++++++++++

Counts of an :math:`n\times m` 2D detectors  are recorded in XML file as below::

  X(1,1)  X(1,2)  X(1,3) ... X(1,m)
  .
  .
  .
  X(n,1)  X(n,2) X(n,3) ... X(n,m)

And the (1,1) position is the bottom left corner of the Anger camera as seen from the sample position.



Output Worskpaces
#################

The output from this algorithm is a MatrixWorskpaces. 

For a 2D detector with :math:`n\times m` pixels, the output MatrixWorkspace
will have :math:`n` spectrum, each of which has a vector of length equal to :math:`m`. 
It can be mapped to the raw data as :math:`WS.readY(i)[j] = X(i+1,j+1)`. 

All experiment information, sample environment devices' readings and monitor counts,
which are recorded in XML files,
are converted to the properties in output MatrixWorkspace's sample log. 


Workflow
########

Algorithm *LoadSpiceXML2DDet* is one of a series of algorithms that are implemented to 
reduced HFIR HB3A data collected from Anger camera. 
It will be called at the first step in the complete workflow.  

Instrument will not be loaded to its output workspace. 


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
