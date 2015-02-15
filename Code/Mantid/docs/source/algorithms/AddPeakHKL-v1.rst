
.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Add a peak in the HKL frame to an existing :ref:`PeaksWorkspace <PeaksWorkspace>`. The OrientedLattice must be provided and the Goniometer should be set correctly before running the algorithm. 

This algorithm will 


Usage
-----
..  Try not to use files in your examples,
    but if you cannot avoid it then the (small) files must be added to
    autotestdata\UsageData and the following tag unindented
    .. include:: ../usagedata-note.txt

**Example - AddPeakHKL**

.. testcode:: AddPeakHKLExample

   # Create a host workspace
   ws = CreateWorkspace(DataX=range(0,3), DataY=(0,2))
   or
   ws = CreateSampleWorkspace()

   wsOut = AddPeakHKL()

   # Print the result
   print "The output workspace has %i spectra" % wsOut.getNumberHistograms()

Output:

.. testoutput:: AddPeakHKLExample

  The output workspace has ?? spectra

.. categories::

