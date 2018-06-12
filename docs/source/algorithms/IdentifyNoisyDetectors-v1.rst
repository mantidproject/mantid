.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

The process for this algorithm is:

-  The standard deviation for each pixel within the pre-set range is
   calculated.
-  The mean value and standard deviation of these standard deviation
   values is calculated.
-  Any detector/pixel for which it's standard deviation value satisfied
   the following conditions is considered to be "noisy":

   -  sdev(pixel) < mean(sdevs) - 3 \* sdev(sdevs)
   -  sdev(pixel) > mean(sdevs) + 3 \* sdev(sdevs)
   -  sdev(pixel) < mean(sdevs) \* 0.0001

This is repeated three times from the second step.

This uses the :ref:`algm-Integration`, :ref:`algm-Power` and
:ref:`algm-Divide` algorithms for the first step.

The lower bound for the integration is currently fixed to 2000.

The upper bound for the integration is currently fixed to 19000.

Usage
-----

**Example: A clean run no noisy detectors**

.. testcode:: ExClearRun

    ws = CreateSampleWorkspace()
    wsOut = IdentifyNoisyDetectors(ws)
    print('The output workspace is filled with {:.0f} for good detectors and 0 for noisy ones.'.format( wsOut.readY(0)[0]))
    sum = SumSpectra(wsOut)
    print("{:.0f} good spectra left".format(sum.readY(0)[0]))

Output:

.. testoutput:: ExClearRun

    The output workspace is filled with 1 for good detectors and 0 for noisy ones.
    200 good spectra left

**Example: With lots of noisy detectors**

.. testcode:: ExNoisy

    ws = CreateSampleWorkspace()
    #make the spectra vary
    for i in range(ws.getNumberHistograms()):
        ws.setY(i, ws.readY(i)*2**i)

    wsOut = IdentifyNoisyDetectors(ws)
    sum = SumSpectra(wsOut)
    print('{:.0f} good spectra left from an original {:.0f}.'.format(sum.readY(0)[0], wsOut.getNumberHistograms()))

Output:

.. testoutput:: ExNoisy

    15 good spectra left from an original 200.

.. categories::

.. sourcelink::
