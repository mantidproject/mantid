.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

FFTSmooth uses the FFT algorithm to create a Fourier transform of a
spectrum, applies a filter to it and transforms it back. The filters
remove higher frequencies from the spectrum which reduces the noise.

This version of the FFTSmooth algorithm has one filter:

Zeroing
#######

-  Filter: "Zeroing"
-  Params: "n" - an integer greater than 1 meaning that the Fourier
   coefficients with frequencies outside the 1/n of the original range
   will be set to zero.


Usage
-----

**Example: Zeroing with Params=2**

.. testcode:: ExFFTSmoothZeroing

    ws = CreateSampleWorkspace(function="Multiple Peaks",XMax=20,BinWidth=0.2,BankPixelWidth=1,NumBanks=1)

    #add a bit of predictable noise
    noiseAmp=0.1
    noiseArray= []
    for i in range(ws.blocksize()):
        noiseAmp = -noiseAmp
        noiseArray.append(noiseAmp)

    for j in range(ws.getNumberHistograms()):
        ws.setY(j,ws.readY(j)+noiseArray)


    wsSmooth = FFTSmooth(ws, Params='2', Version=1)

    print("bin Orig  Smoothed")
    for i in range (0,100,10):
        print("{}  {:.2f}  {:.2f}".format(i, ws.readY(0)[i], wsSmooth.readY(0)[i]))


.. figure:: /images/FFTSmoothZeroing.png
    :align: right
    :height: 280px

Output:

.. testoutput:: ExFFTSmoothZeroing

    bin Orig  Smoothed
    0  0.20  0.30 
    10  0.20  0.30 
    20  0.37  0.47 
    30  10.20  10.30 
    40  0.37  0.47 
    50  0.20  0.30 
    60  8.20  8.30 
    70  0.20  0.30 
    80  0.20  0.30 
    90  0.20  0.30 

.. categories::

.. sourcelink::
