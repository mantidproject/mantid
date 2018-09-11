.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

FFTSmooth uses the FFT algorithm to create a Fourier transform of a
spectrum, applies a filter to it and transforms it back. The filters
remove higher frequencies from the spectrum which reduces the noise.

The second version of the FFTSmooth algorithm has two filters:

Zeroing
#######

-  Filter: "Zeroing"
-  Params: "n" - an integer greater than 1 meaning that the Fourier
   coefficients with frequencies outside the 1/n of the original range
   will be set to zero.

Butterworth
###########

-  Filter: "Butterworth"
-  Params: A string containing two positive integer parameters separated
   by a comma, such as 20,2.

   "n"- the first integer, specifies the cutoff frequency for the filter,
   in the same way as for the "Zeroing" filter. That is, the cutoff is at
   m/n where m is the original range. "n" is required to be strictly more
   than 1.

   "order"- the second integer, specifies the order of the filter. For low
   order values, such as 1 or 2, the Butterworth filter will smooth the
   data without the strong "ringing" artifacts produced by the abrupt
   cutoff of the "Zeroing" filter. As the order parameter is increased, the
   action of the "Butterworth" filter will approach the action of the
   "Zeroing" filter.

For both filter types, the resulting spectrum has the same size as the
original one.

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


    wsSmooth = FFTSmooth(ws, Params='2')

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


**Example: Using the  Butterworth filter**

.. testcode:: ExFFTSmoothButterworth

    ws = CreateSampleWorkspace(function="Multiple Peaks",XMax=20,BinWidth=0.2,BankPixelWidth=1,NumBanks=3)

    #add a bit of predictable noise
    noiseAmp=0.1
    noiseArray= []
    for i in range(ws.blocksize()):
        noiseAmp = -noiseAmp
        noiseArray.append(noiseAmp)

    for j in range(ws.getNumberHistograms()):
        ws.setY(j,ws.readY(j)+noiseArray)


    wsButter2_2 = FFTSmooth(ws, Filter="Butterworth", Params='2,2', AllSpectra=True)
    wsButter5_2 = FFTSmooth(ws, Filter="Butterworth", Params='5,2', AllSpectra=True)
    wsButter20_2 = FFTSmooth(ws, Filter="Butterworth", Params='20,2', AllSpectra=True)

    print("bin Orig  2_2   5_2   20_2")
    for i in range (0,100,10):
        print("{}  {:.2f}  {:.2f}  {:.2f}  {:.2f}".format(i, ws.readY(0)[i], wsButter2_2.readY(0)[i], wsButter5_2.readY(0)[i], wsButter20_2.readY(0)[i]))


.. figure:: /images/FFTSmoothZeroingButter.png
    :align: right
    :height: 280px

Output:

.. testoutput:: ExFFTSmoothButterworth

    bin Orig  2_2   5_2   20_2
    0  0.20  0.29  0.30  -0.05
    10  0.20  0.29  0.30  0.44
    20  0.37  0.46  0.43  2.49
    30  10.20  10.26  9.59  4.58
    40  0.37  0.46  0.43  2.63
    50  0.20  0.29  0.16  1.77
    60  8.20  8.20  7.05  2.74
    70  0.20  0.29  0.16  1.48
    80  0.20  0.29  0.30  0.39
    90  0.20  0.29  0.30  0.20



Usage
-----

.. testcode::

  # Create a workspace
  ws = CreateSampleWorkspace()

  # Apply the Butterworth filter to all spectra
  smooth = FFTSmooth( ws, Filter='Butterworth', Params='5,2', AllSpectra=True )

.. categories::

.. sourcelink::
