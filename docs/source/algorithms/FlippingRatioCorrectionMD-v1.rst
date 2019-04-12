
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

TODO: Enter a full rst-markup description of your algorithm here.


Usage
-----
..  Try not to use files in your examples,
    but if you cannot avoid it then the (small) files must be added to
    autotestdata\UsageData and the following tag unindented
    .. include:: ../usagedata-note.txt

**Example - FlippingRatioCorrectionMD**

.. testcode:: FlippingRatioCorrectionMDExample

   # Create a host workspace
   ws = CreateWorkspace(DataX=range(0,3), DataY=(0,2))
   or
   ws = CreateSampleWorkspace()

   wsOut = FlippingRatioCorrectionMD()

   # Print the result
   print "The output workspace has %%i spectra" %% wsOut.getNumberHistograms()

Output:

.. testoutput:: FlippingRatioCorrectionMDExample

  The output workspace has ?? spectra

.. categories::

.. sourcelink::

