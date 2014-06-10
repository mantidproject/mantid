.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Workflow algorithm that loads HFIR SANS data using the
`LoadSpice2D <http://www.mantidproject.org/LoadSpice2D>`_
algorithm and applies basic corrections to the workspace. Those include:

- Moving the detector at its proper position in Z.

- Moving the detector according to the beam center.

- Reading in the following wavelength and wavelength spread from the file and storing them in the logs.

- Compute the source-sample distance according to the number of guides and store it in the logs.

- Compute the beam diameter according to the distances and apertures, and store it in the logs:
  :math:`D_{beam} =  (A_{source}+A_{sample})\frac{SDD}{SSD} + A_{sample}`

This algorithm is rarely called directly. It is called by 
`HFIRSANSReduction <http://www.mantidproject.org/HFIRSANSReduction>`_, which will
pass along the relevant beam center information through the *ReductionProperties* property.


Usage
-----

.. include:: ../usagedata-note.txt

**Example - Load a BioSANS data file:**

.. testcode:: ExHfirLoad

   workspace = HFIRLoad('BioSANS_empty_cell.xml')
   r= mtd['workspace'].run()
   print "SDD = %g" % r.getProperty('sample-detector-distance').value
   

Output:

.. testoutput:: ExHfirLoad

   SDD = 6000
   
.. categories::
