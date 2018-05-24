.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This is an algorithm for Fourier transfom of real data. It uses the GSL
routines gsl\_fft\_real\_transform and gsl\_fft\_halfcomplex\_inverse.
The result of a forward transform is a three-spectra workspace with the
real and imaginary parts of the transform in position 0 and 1
correspondingly. The third spectrum has the modulus of the transform (:math:`\sqrt{Re^2+Im^2}`).
Only positive frequencies are given and as a result the
output spectra are twice as short as the input one.

An input workspace for backward transform must have the form of the
output workspace of the forward algorithm, i.e. its first two spectra must have
the real part and the imaginary parts. The output workspace contains a single spectrum with the real
inverse transform.

The forward transform doesn't use the absolute values of the X axis but rather assumes that the data starts at
X = 0 and ends at X = XMax - XMin, where XMin and XMax are the lower and the upper limits on the X axis.
As a result the output is a transform of the input spectrum shifted along the X axis so that the
first bin is centered at 0.

The output of the backward transform always starts at X = 0.

Usage
-----

.. testcode::

  import numpy as np

  # Create a workspace with a Gaussian peak in the centre.
  ws = CreateSampleWorkspace(Function='User Defined',UserDefinedFunction='name=Gaussian,Height=1,PeakCentre=0,Sigma=1',XMin=-10,XMax=10,BinWidth=0.1)
  # Forward transform its 11-th spectrum
  transform = RealFFT( ws, 10, "Forward")
  # Backward transform must look like the original spectrum only shifted along the X axis
  ws_back = RealFFT(transform, Transform="Backward")

  # Check the outputs

  # Check the sizes
  print('Number of bins in the input workspace    {}'.format(ws.blocksize()))
  print('Number of bins in the forward transform  {}'.format(transform.blocksize()))
  print('Number of bins in the backward transform {}'.format(ws_back.blocksize()))

  # Check the X axes
  print('Input starts at {:.1f} , ends at {:.1f} , the width is {:.1f}'.format(
        ws.readX(10)[0], ws.readX(10)[-1], ws.readX(10)[-1] - ws.readX(10)[0]))
  print('Forward starts at  {:.1f} , ends at {:.2f} , the width is {:.2f}'.format(
        transform.readX(0)[0], transform.readX(0)[-1], transform.readX(0)[-1] - transform.readX(0)[0]))
  print('Backward starts at {:.1f} , ends at {:.1f} , the width is {:.1f}'.format(
        ws_back.readX(0)[0], ws_back.readX(0)[-1], ws_back.readX(0)[-1] - ws_back.readX(0)[0]))

  # Check that the backward transform restores the original data.
  # The input spetrum values
  y10 = ws.readY(10)
  # The spectrum returned from the backward RealFFT
  y_back = ws_back.readY(0)
  # Check that they are almost equal.
  # Using numpy array calculations show that all elements of arrays y_back and y10 are very close
  print(np.all(np.abs(y_back - y10) < 1e-15))
  # but not equal
  print(np.all(y_back == y10))


Output
######

.. testoutput::

  Number of bins in the input workspace    200
  Number of bins in the forward transform  101
  Number of bins in the backward transform 200
  Input starts at -10.0 , ends at 10.0 , the width is 20.0
  Forward starts at  0.0 , ends at 5.05 , the width is 5.05
  Backward starts at 0.0 , ends at 20.0 , the width is 20.0
  True
  False

.. categories::

.. sourcelink::
