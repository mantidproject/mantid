
.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Given an input workspace containing data with a background and a table of 
user-selected points defining the background, creates a new workspace 
containing background data that can be subtracted from the original data.

Typical use case
################

#. Spectrum has several peaks on top of a large, irregularly shaped background.
#. Plot spectrum and use *Data > Draw Data Points* in MantidPlot to define the background.
   Clicking anywhere outside of the graph will show a table with the points defined by clicking.
#. Convert table to a TableWorkspace (*Table > Convert to TableWorkspace*)
#. Run this algorithm, giving it the original data and the background table.
   It will return a workspace filled with background data.
#. Use :ref:`algm-Minus` to subtract the returned workspace from the original data workspace. 


Usage
-----
..  Try not to use files in your examples,
    but if you cannot avoid it then the (small) files must be added to
    autotestdata\UsageData and the following tag unindented
    .. include:: ../usagedata-note.txt

**Example - CreateUserDefinedBackground**

.. testcode:: CreateUserDefinedBackgroundExample

   # Create a host workspace
   ws = CreateWorkspace(DataX=range(0,3), DataY=(0,2))
   or
   ws = CreateSampleWorkspace()

   wsOut = CreateUserDefinedBackground()

   # Print the result
   print "The output workspace has %i spectra" % wsOut.getNumberHistograms()

Output:

.. testoutput:: CreateUserDefinedBackgroundExample

  The output workspace has ?? spectra

.. categories::

.. sourcelink::

