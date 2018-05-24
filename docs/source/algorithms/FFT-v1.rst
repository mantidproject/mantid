.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

The FFT algorithm performs discrete Fourier transform of complex data
using the Fast Fourier Transform algorithm. It uses the GSL Fourier
transform functions to do the calculations. Due to the nature of the
fast fourier transform the input spectra must have linear x axes. If the
imaginary part is not set the data is considered real. The "Transform"
property defines the direction of the transform: direct ("Forward") or
inverse ("Backward").

Note that it is assumed that the input data has the x origin in the
middle of the x-value range. It means that for the data defined on
an interval [A,B] the output :math:`F(\xi_k)` must be multiplied by 
:math:`e^{-2\pi ix_0\xi_k}`, where :math:`x_0=\tfrac{1}{2}(A+B)` and
:math:`\xi_k` is the frequency. This can be achieved by using the 
input parameter *Shift*, which applies a phase shift to the transform,
or by setting *AutoShift* on to do this automatically.

Details
-------

The Fourier transform of a complex function :math:`f(x)` is defined by
equation:

.. math:: F(\xi)=\int_{-\infty}^\infty f(x)e^{-2\pi ix\xi} dx

For discrete data with equally spaced :math:`x_n` and only non-zero on
an interval :math:`[A,B]` the integral can be approximated by a sum:

.. math:: F(\xi)=\Delta x\sum_{n=0}^{N-1}f(x_n)e^{-2\pi ix_n\xi}

Here :math:`\Delta x` is the bin width, :math:`x_n=A+\Delta xn`. If we
are looking for values of the transformed function :math:`F` at discrete
frequencies :math:`\xi_k=\Delta\xi k` with

.. math:: \Delta\xi=\frac{1}{B-A}=\frac{1}{L}=\frac{1}{N\Delta x}

the equation can be rewritten:

.. math:: F_k=e^{-2\pi iA\xi_k}\Delta x\sum_{n=0}^{N-1}f_ne^{-\tfrac{2\pi i}{N}nk}

Here :math:`f_n=f(x_n)` and :math:`F_k=F(\xi_k)`. The formula

.. math:: \tilde{F}_k=\Delta x\sum_{n=0}^{N-1}f_ne^{-\tfrac{2\pi i}{N}nk}

is the Discrete Fourier Transform (DFT) and can be efficiently evaluated
using the Fast Fourier Transform algorithm. The DFT formula calculates
the Fourier transform of data on the interval :math:`[0,L]`. It should
be noted that it is periodic in :math:`k` with period :math:`N`. If we
also assume that :math:`f_n` is also periodic with period :math:`N` the
DFT can be used to transform data on the interval :math:`[-L/2,L/2]`. To
do this we swap the two halves of the data array :math:`f_n`. If we
denote the modified array as :math:`\bar{f}_n`, its transform will be

.. math:: \bar{F}_k=\Delta x\sum_{n=0}^{N-1}\bar{f}_ne^{-\tfrac{2\pi i}{N}nk}

The Mantid FFT algorithm returns the complex array :math:`\bar{F}_K` as
Y values of two spectra in the output workspace, one for the real and
the other for the imaginary part of the transform. The X values are set
to the transform frequencies and have the range approximately equal to
:math:`[-N/L,N/L]`. The actual limits depend slightly on whether
:math:`N` is even or odd and whether the input spectra are histograms or
point data. The variations are of the order of :math:`\Delta\xi`. The
zero frequency is always in the bin with index :math:`k=int(N/2)`.

The X values of the input data must be evenly spaced for the FFT algorithm
to work (all bin widths must be the same). If they contain small rounding
errors, this requirement can be relaxed by setting the *AcceptXRoundingErrors*
property, which will continue to process the data even if the spacings between
different X values are unequal. Large differences in the bin widths will still
produce a warning.

If the X values are not centred on zero, the calculated phase will be wrong.
The *Shift* property can be used to correct this - the value supplied is the value
that should be added to the X values to centre the X axis. Setting the *AutoShift*
property will automatically apply the correct shift (overriding any manually supplied
*Shift*).

The algorithm works internally with point data - if a histogram workspace is provided as input, the bin edges are converted to points automatically inside the algorithm.
The output transform is always a point data workspace. (It can be subsequently converted back to a histogram workspace using :ref:`algm-ConvertToHistogram`, if required).

Example 1
#########

In this example the input data were calculated using function
:math:`\exp(-(x-1)^2)` in the range [-5,5].

.. figure:: /images/FFTGaussian1.png
   :alt: Gaussian

   Gaussian
.. figure:: /images/FFTGaussian1FFT.png
   :alt: FFT of a Gaussian

   FFT of a Gaussian

Because the :math:`x=0` is in the middle of the data array the transform
shown is the exact DFT of the input data.

Example 2
#########

In this example the input data were calculated using function
:math:`\exp(-x^2)` in the range [-6,4].

.. figure:: /images/FFTGaussian2.png
   :alt: Gaussian

   Gaussian

.. figure:: /images/FFTGaussian1FFT.png
   :alt: FFT of a Gaussian

   FFT of a Gaussian

Because the :math:`x=0` is not in the middle of the data array the
transform shown includes a shifting factor of :math:`\exp(2\pi i\xi)`.
To remove it the output must be multiplied by :math:`\exp(-2\pi i\xi)`.
The corrected transform will be:

.. figure:: /images/FFTGaussian2FFT.png
   :alt: FFT of a Gaussian

   FFT of a Gaussian

It should be noted that in a case like this, i.e. when the input is a
real positive even function, the correction can be done by finding the
transform's modulus :math:`(Re^2+Im^2)^{1/2}`. The output workspace
includes the modulus of the transform.

Output
------

The output workspace for a direct ("Forward") transform contains either
three or six spectra, depending on whether the input function is complex
or purely real. If the input function has an imaginary part, the
transform is written to three spectra with indexes 0, 1, and 2. Indexes
0 and 1 are the real and imaginary parts, while index 2 contains the
modulus :math:`\sqrt{Re^2+Im^2}`. If the input function does not contain
an spectrum for the imaginary part (purely real function), the actual
transform is written to spectra with indexes 3 and 4 which are the real
and imaginary parts, respectively. The last spectrum (index 5) has the
modulus of the transform. The spectra from 0 to 2 repeat these results
for positive frequencies only.

Output for the case of input function containing imaginary part:

+-------------------+------------------------------+
| Workspace index   | Description                  |
+===================+==============================+
| 0                 | Complete real part           |
+-------------------+------------------------------+
| 1                 | Complete imaginary part      |
+-------------------+------------------------------+
| 2                 | Complete transform modulus   |
+-------------------+------------------------------+

Output for the case of input function containing no imaginary part:

+-------------------+----------------------------------------+
| Workspace index   | Description                            |
+===================+========================================+
| 0                 | Real part, positive frequencies        |
+-------------------+----------------------------------------+
| 1                 | Imaginary part, positive frequencies   |
+-------------------+----------------------------------------+
| 2                 | Modulus, positive frequencies          |
+-------------------+----------------------------------------+
| 3                 | Complete real part                     |
+-------------------+----------------------------------------+
| 4                 | Complete imaginary part                |
+-------------------+----------------------------------------+
| 5                 | Complete transform modulus             |
+-------------------+----------------------------------------+

The output workspace for an inverse ("Backward") transform has 3 spectra
for the real (0), imaginary (1) parts, and the modulus (2).

+-------------------+------------------+
| Workspace index   | Description      |
+===================+==================+
| 0                 | Real part        |
+-------------------+------------------+
| 1                 | Imaginary part   |
+-------------------+------------------+
| 2                 | Modulus          |
+-------------------+------------------+

Usage
-------

**Example: Applying FFT algorithm**

.. testcode:: FFTBackwards

   #Create Sample Workspace 
   ws = CreateSampleWorkspace(WorkspaceType = 'Event', NumBanks = 1, Function = 'Exp Decay', BankPixelWidth = 1, NumEvents = 100)

   #apply the FFT algorithm - note output is point data
   outworkspace = FFT(InputWorkspace = ws, Transform = 'Backward')

   print("DataX(0)[1] equals DataX(0)[99]? : " + str((round(abs(outworkspace.dataX(0)[1]), 3)) == (round(outworkspace.dataX(0)[99], 3))))
   print("DataX(0)[10] equals DataX(0)[90]? : " + str((round(abs(outworkspace.dataX(0)[10]), 3)) == (round(outworkspace.dataX(0)[90], 3))))
   print("DataX((0)[50] equals 0? : " + str((round(abs(outworkspace.dataX(0)[50]), 3)) == 0))
   print("DataY(0)[40] equals DataY(0)[60]? : " + str((round(abs(outworkspace.dataY(0)[40]), 5)) == (round(outworkspace.dataY(0)[60], 5))))

Output:

.. testoutput:: FFTBackwards
	
   DataX(0)[1] equals DataX(0)[99]? : True
   DataX(0)[10] equals DataX(0)[90]? : True
   DataX((0)[50] equals 0? : True
   DataY(0)[40] equals DataY(0)[60]? : True


.. categories::

.. sourcelink::
    :h: Framework/Algorithms/inc/MantidAlgorithms/FFT.h
    :cpp: Framework/Algorithms/src/FFT.cpp
