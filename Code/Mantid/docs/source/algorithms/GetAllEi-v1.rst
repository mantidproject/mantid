.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Algorithm finds the estimate for all incident energies allowed by chopper system of an inelastic instrument and returns a workspace,
with the estimates for positions, heights and width of incident energies provided by the choppers. These estimates can be used as guess value for 
:ref:`algm-GetEi` algorithm or as inputs for a peak fitting procedure.

Algorithm performs number of steps to identify the values requested:

* It takes appropriate log names from insrument definition file (IDF), namely chopper-position component and calculates last chopper speed and delay as average
of the filtered log values. Guess chopper opening times are calculated from chopper speed and delay time. The "chopper-position" component with appopriate properties 
has to be present in IDF  for this algorithm to work. See ISIS MARI or MAPS instrument definition files for example of "chopper-position" component.

* Algorithm takes mininal instrument resolution estimate and searches for real peaks around guess values above within 4 sigma of this resolution interval.

* If peaks are found, the algorithm performs running averages over signal in the appropriate time interval until first derivative 
of the signal has only one zero. This value is accepted as the guess energy postion and the the distance between closest to 
the guess energy zeros of the second derivative are accpted as the guess values for the peak width. The peak amplitude 
is estimated from the integral intensity of the signal, assuming that the peak shape is Gaussian.

* The similar procedure is performed for second monitors. The peak is accepted as real only if the peak width is within the limits of the instrument resolution and 
the distance between peaks positions on two monitors (on energy scalse) is smaller then two sigma.

Algorithm returns matrix workspace contatingn single sepctra, with x-value representing peak positions, y-values: peak heigths and the error: peak width.

Used Subalgorithms
------------------
The algorithm uses :ref:`Unit Factory <Unit Factory>` and :ref:`algm-ConvertUnits` algorithm 
to convert units from TOF to energy. 

Usage
-----

.. include:: ../usagedata-note.txt

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

.. sourcelink::
