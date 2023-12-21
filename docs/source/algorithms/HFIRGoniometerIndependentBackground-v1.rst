.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm is used to generate a background for HFIR monochromatic diffraction data. This algorithm wraps
`Scipy.ndimage.percentile_filter <https://docs.scipy.org/doc/scipy/reference/generated/scipy.ndimage.percentile_filter.html>`_
to generate the background for the input workspace. In the case that BackgroundWindowSize is -1,
`Numpy.percentile <https://numpy.org/doc/stable/reference/generated/numpy.percentile.html>`_ is used to generate the background as
it is much faster.



Usage
-----

.. testcode::

   # create workspace
   import numpy as np
   signal = np.random.randint(low=0, high=10, size=(100,100,100))
   workspace = CreateMDHistoWorkspace(SignalInput=signal,
                                      ErrorInput=np.ones_like(signal),
                                      Dimensionality=3,
                                      Extents='0,10,0,10,0,10',
                                      Names='x,y,z',
                                      NumberOfBins='100,100,100',
                                      Units='number,number,number',
                                      OutputWorkspace='output')



   # Perform the background interpolation
   outputWS = HFIRGoniometerIndependentBackground(workspace, BackgroundWindowSize=10)

   # Check output
   print("Shape of the resulting Signal is: {}".format(outputWS.getSignalArray().shape))

Output:

.. testoutput::

   Shape of the resulting Signal is: (100, 100, 100)


.. categories::

.. sourcelink::
