.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Uses :math:`E=\frac{1}{2}mv^2`
to calculate the energy of neutrons leaving the
source. The velocity is calculated from the time it takes for the
neutron pulse to travel between the two monitors whose spectra were
specified. If no spectra are specified, the algorithm will use the
defaults for the instrument.

An initial energy guess is required for the algorithm to find the
correct peak. The analysis will be done on the highest peak that is
within 8% of the estimated TOF given by the estimate. If no initial
guess is given, the algorithm will try to get it from the workspace,
from a sample log variable called *EnergyRequest*.

Not all neutrons arrive at the monitors at the same time because their
kinetic energies, and therefore velocities, are all different. The time
of arrival of the neutron pulse is taken to be the mean of the two half
peak height locations. The half height points are found as follows:

#. the peak height is the largest number of counts above the background
   in any bin in the window
#. the half height is half the above number
#. examine bins to the left of the bin with the highest number of counts
   looking for a bin with less than half that number above background
#. interpolate between this point bin and the one immediately previous
   to find the first half height location
#. repeat the steps 3 and 4 looking to the right of the highest point to
   get the second half height point
#. the mean of the X-values of the two half height points is the TOF
   arrival time of the neutrons

The above process is illustrated on a peak is shown below in the image
below.

.. image:: /images/Monitorspect_getei.jpg
    :align: center
    :width: 618px
    :alt: Monitor Peak

The distances between the monitors are read from the instrument
definition file. It is assumed that the source and the monitors all lie
on one line and that the monitors have the same delay time.

Usage
-----

**Example: Calculating the Ei**

.. testcode:: calcEi
    
    import numpy as np
    import math

    #create a workspace wth two pixels to act as monitors
    ws = CreateSampleWorkspace(bankPixelWidth=1,binWidth=10)

    #set the data values
    peakOneCentre = 8500.0
    sigmaSqOne = 250.0*250.0
    peakTwoCentre = 10800.0
    sigmaSqTwo = 50*50
    peakOneHeight = 3000.0
    peakTwoHeight = 1000.0
    xArray = []
    yArray0 = []
    yArray1 = []
    for i in range (ws.blocksize()):
        xValue = 5.0 + 5.5*i
        xArray.append(xValue)
        yArray0.append(peakOneHeight * math.exp(-0.5*pow(xValue - peakOneCentre, 2.)/sigmaSqOne))
        yArray1.append(peakTwoHeight * math.exp(-0.5*pow(xValue - peakTwoCentre, 2.)/sigmaSqTwo))
    xArray.append(5.0 + 5.5*ws.blocksize()) 
    
    ws.setX(0, np.array(xArray))
    ws.setX(1, np.array(xArray))   
    ws.setY(0, np.array(yArray0))
    ws.setY(1, np.array(yArray1))

    (ei, firstMonitorPeak, FirstMonitorIndex, tzero) = GetEi(ws,Monitor1Spec=1,Monitor2Spec=2,EnergyEstimate=15.0)

    print "ei: %.2f" % ei
    print "firstMonitorPeak: %.2f" % firstMonitorPeak
    print "FirstMonitorIndex: %i" % FirstMonitorIndex
    print "tzero: %.2f" % tzero

Output:

.. testoutput:: calcEi
    :options: +NORMALIZE_WHITESPACE

    ei: 24.99
    firstMonitorPeak: 8516.03
    FirstMonitorIndex: 0
    tzero: 1655.69

**Example: Fixing the Ei**

.. testcode:: fixEi
    
    ws = CreateSampleWorkspace(bankPixelWidth=1,binWidth=10)

    (ei, firstMonitorPeak, FirstMonitorIndex, tzero) = GetEi(ws,Monitor1Spec=1,Monitor2Spec=2,EnergyEstimate=15.0,FixEi=True)

    print "ei: %.2f" % ei
    print "firstMonitorPeak: %.2f" % firstMonitorPeak
    print "FirstMonitorIndex: %i" % FirstMonitorIndex
    print "tzero: %.2f" % tzero

Output:

.. testoutput:: fixEi
    :options: +NORMALIZE_WHITESPACE

    ei: 15.00
    firstMonitorPeak: 8854.69
    FirstMonitorIndex: 0
    tzero: 0.00

.. categories::
