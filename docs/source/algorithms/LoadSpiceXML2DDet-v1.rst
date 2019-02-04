.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm is to import SPICE-generated XML file and binary file that
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


Format of SPICE binary data file
################################

The SPICE binary data file contains 2 + N unsigned integers, where N is the number of detector pixels.

Here is the specification of the SPICE 2D detector binary file.

  int0: number of rows in 2D detector
  int1: number of columns in 2D detector
  int2: counts of pixel on the lower left corner facing to detector from sample
  int3: counts of pixel just above the lower left corner
  .
  .
  .
  int(N+2): counts of pixel on the upper right corner

Note: int0 x int1 must be equal to N.


HB3A instrument facts
#####################

HB3A has 1 detector with :math:`256 \times 256` pixels.

 - Pixel: width = :math:`2 \times 9.921875e-05` m, height = :math:`2 \times 9.921875e-05` m, depth = 0.0001 m.
 - Detector:


Output Worskpaces
#################

The output from this algorithm is a MatrixWorskpaces.

MatrixWorskpace with instrument loaded
++++++++++++++++++++++++++++++++++++++

For a 2D detector with :math:`n\times m` pixels, the output MatrixWorkspace
will have :math:`n \times m` spectrum.
Each spectrum has 1 data point corresponding to 1 detector's count.

All experiment information, sample environment devices' readings and monitor counts,
which are recorded in XML files,
are converted to the properties in output MatrixWorkspace's sample log.

MatrixWorkspace without instrument loaded
+++++++++++++++++++++++++++++++++++++++++

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
It will be called next to *LoadSpiceAscii* to load the detector's reading.

Usage
-----

**Example - load a HB3A SPICE .xml file without loading instrument:**

.. testcode:: ExLoadHB3AXMLData

  # Load data by LoadSpiceXML2DDet()
  LoadSpiceXML2DDet(Filename='HB3A_exp355_scan0001_0522.xml',
      OutputWorkspace='s0001_0522', DetectorGeometry='256,256',
      LoadInstrument=False)

  # Access output workspace and print out some result
  ws = mtd["s0001_0522"]

  print("Number of spectrum = {}.".format(ws.getNumberHistograms()))
  for i, j in [(0, 0), (255, 255), (136, 140), (143, 140)]:
      print("Y[{:<3}, {:<3}] = {:.5f}".format(i, j, ws.readY(i)[j]))

.. testcleanup:: ExLoadHB3AXMLData

  ws = mtd["s0001_0522"]
  DeleteWorkspace(Workspace=str(ws))

Output:

.. testoutput:: ExLoadHB3AXMLData

  Number of spectrum = 256.
  Y[0  , 0  ] = 0.00000
  Y[255, 255] = 0.00000
  Y[136, 140] = 0.00000
  Y[143, 140] = 1.00000

.. categories::

.. sourcelink::
