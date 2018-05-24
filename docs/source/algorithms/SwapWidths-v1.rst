.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm is used mainly in tandem with the output of BayesQuasi when the data contains multiple peaks.
Quasi Lines 


Usage
-----

**Example - a basic example using SwapWidths.**

.. testcode:: SwapWidthsExample

    # Load workspace to perform swap with
    ws = Load(Filename='IN16B_125878_QLd_Result.nxs')

    # Perform Swap (data crosses at bin 5)
    result = SwapWidths(InputWorkspace=ws, SwapPoint=5)

    # Print the result
    print("The resulting workspace has %d bins and %d histograms." % (result.blocksize(), result.getNumberHistograms()))


Output:

.. testoutput:: SwapWidthsExample
    :options: +NORMALIZE_WHITESPACE
	
    The resulting workspace has 16 bins and 2 histograms.
	

.. categories::

.. sourcelink::
