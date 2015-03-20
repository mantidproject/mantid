
.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Provides smoothing of :ref:`MDHistoWorkspaces <MDHistoWorkspace>`__ in n-dimensions. The WidthVector relates to the number of pixels to include in the width for each dimension. *WidthVector* **must contain entries that are odd numbers**.


Usage
-----
..  Try not to use files in your examples,
    but if you cannot avoid it then the (small) files must be added to
    autotestdata\UsageData and the following tag unindented
    .. include:: ../usagedata-note.txt

**Example - SmoothMD**

.. testcode:: SmoothMDExample

   # Create a host workspace
   ws = CreateWorkspace(DataX=range(0,3), DataY=(0,2))
   or
   ws = CreateSampleWorkspace()

   wsOut = SmoothMD()

   # Print the result
   print "The output workspace has %i spectra" % wsOut.getNumberHistograms()

Output:

.. testoutput:: SmoothMDExample

  The output workspace has ?? spectra

.. categories::

