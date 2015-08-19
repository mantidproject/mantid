
.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

An algorithm designed mainly as a sequential call to PlotPeakByLogValue 
but used within the ConvFit tab within the Indirect Analysis interface 
to fit Convolution Functions.


Usage
-----

**Example - ConvolutionFitSequential**

.. testcode:: ConvolutionFitSequentialExample

   # Create a host workspace
   ws = CreateWorkspace(DataX=range(0,3), DataY=(0,2))
   or
   ws = CreateSampleWorkspace()

   wsOut = ConvolutionFitSequential()

   # Print the result
   print "The output workspace has %i spectra" % wsOut.getNumberHistograms()

Output:

.. testoutput:: ConvolutionFitSequentialExample

  The output workspace has ?? spectra

.. categories::

.. sourcelink::

