.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm iterates the :ref:`algm-FFT` algorithm on each spectrum of
InputWorkspace, computing the Fourier Transform and storing the
transformed spectrum in OutputWorkspace. If InputImagWorkspace is also
passed, then the pair spectrum *i* of InputWorkspace (real) and spectrum
*i* of InputImagWorkspace (real) are taken together as spectrum *i* of a
complex workspace, on which :ref:`algm-FFT` is applied.

The FFTPart parameter specifies which transform is selected from the
output of the :ref:`algm-FFT` algorithm:

For the case of input containing real and imaginary workspaces:

+-----------+------------------------------+
| FFTPart   | Description                  |
+===========+==============================+
| 0         | Complete real part           |
+-----------+------------------------------+
| 1         | Complete imaginary part      |
+-----------+------------------------------+
| 2         | Complete transform modulus   |
+-----------+------------------------------+

For the case of input containing no imaginary workspace:

+-----------+----------------------------------------+
| FFTPart   | Description                            |
+===========+========================================+
| 0         | Real part, positive frequencies        |
+-----------+----------------------------------------+
| 1         | Imaginary part, positive frequencies   |
+-----------+----------------------------------------+
| 2         | Modulus, positive frequencies          |
+-----------+----------------------------------------+
| 3         | Complete real part                     |
+-----------+----------------------------------------+
| 4         | Complete imaginary part                |
+-----------+----------------------------------------+
| 5         | Complete transform modulus             |
+-----------+----------------------------------------+


Usage
-----

**Example - Extract FFT spectrum from an exponential and a gaussian.**

.. testcode:: Ex

    import numpy

    # Funtions x and y defined over the time domain: z(t) = x(t) + i * y(t)
    dt=0.001
    t=numpy.arange(-1,1,dt)

    # Exponential decay in [-1,1] for the real part
    tau=0.05
    x=numpy.exp(-numpy.abs(t)/tau)
    wsx = CreateWorkspace(t,x)

    # Gaussian decay in [-1,1] for the imaginary part
    sigma=0.05
    y=numpy.exp(-t*t/(2*sigma*sigma))
    wsy = CreateWorkspace(t,y)

    # Extract the FFT spectrum from z(t) = x(t) + i * y(t)
    wsfr = ExtractFFTSpectrum(InputWorkspace=wsx, InputImagWorkspace=wsy, FFTPart=0)  # real part
    wsfi = ExtractFFTSpectrum(InputWorkspace=wsx, InputImagWorkspace=wsy, FFTPart=1)  # imaginary

    # Test the real part with a fitting to the expected Lorentzian
    myFunc = 'name=Lorentzian,Amplitude=0.05,PeakCentre=0,FWHM=6,ties=(PeakCentre=0)'
    fitStatus, chiSq, covarianceTable, paramTable, fitWorkspace = Fit(Function=myFunc, InputWorkspace='wsfr', StartX=-40, EndX=40, CreateOutput=1)
    print("Theoretical FWHM = 1/(pi*tau)=6.367 -- Fitted FWHM value is: %.3f" % paramTable.column(1)[2])

    # Test the imaginary part with a fitting to the expected Gaussian
    myFunc = 'name=Gaussian,Height=0.1,PeakCentre=0,Sigma=3.0, ties=(PeakCentre=0)'
    fitStatus, chiSq, covarianceTable, paramTable, fitWorkspace = Fit(Function=myFunc, InputWorkspace='wsfi', StartX=-15, EndX=15, CreateOutput=1)
    print("Theoretical Sigma = 1/(2*pi*sigma)=3.183 -- Fitted Sigma value is: %.3f" % paramTable.column(1)[2])

Output:

.. testoutput:: Ex

    Theoretical FWHM = 1/(pi*tau)=6.367 -- Fitted FWHM value is: 6.367
    Theoretical Sigma = 1/(2*pi*sigma)=3.183 -- Fitted Sigma value is: 3.183

.. categories::
