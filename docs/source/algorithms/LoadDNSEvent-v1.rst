.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------


This algorithm loads a DNS mesytec psd listmode file into an :ref:`EventWorkspace <EventWorkspace>`. 

**Output**

    :ref:`EventWorkspace <EventWorkspace>` with the X-axis in the Time-of-flight units.

**ChopperChannel**
is the input channel to wich the chopper was connected, must also be set in mesydaq gui.
It can be set to integers between 0 and 4. If the value is not set or is set to 0, the default value in the instrument definition file is used which is 2.


**DiscardPreChopperEvents**:
If set to *true* events with a timestamp before the first chopper timestamp are ignored, since they have no valid TOF. 
If the dataset is elastic they can be included, since only the position and not the TOF is used.


**SetBinBoundary**:
If set to *true* the bin boundaries of the spectrum histrograms are set to include all events in one bin (0 to maximum TOF).
It can be turned off for faster loading, then the histograms are empty, with both boundaries close to 0.
The binning can then later be reproduced by applying  :ref:`Rebin <Rebin>` with the paraemter *Params='0, {0}, {0}'.format(eventWS.getTofMax())*.


Restrictions
------------

- This algorithm only supports the *DNS* instrument.

- This algorithm loads raw TOF data, without normalization and without considering the detectorbank position. 


Usage
-----

**Example 1 - Load TOF PSD data:**

.. testcode:: LoadDNSEventex1

    # data file.
    filename = "C:/data/psd_data/00796651.mdat"

    eventWS = LoadDNSEvent(InputFile=filename, ChopperChannel='2', MonitorChannel='0')
    print("Number of events: {}".format(eventWS.getNumberEvents()))
    print("Maximum time of flight: {}".format(eventWS.getTofMax()))
    print("Number of detector pixels: {}".format(eventWS.getNumberHistograms()))
    print("Number of bins: {}".format(eventWS.blocksize()))
    
    # rebin spectra:
    test2 = Rebin(InputWorkspace='eventWS', Params='0, 100, {0}'.format(eventWS.getTofMax()))
    print("Number of bins after rebinning: {}".format(eventWS.blocksize()))


**Output:**

.. testoutput:: LoadDNSEventex1
   :options: +NORMALIZE_WHITESPACE

    Number of events: 100
    Maximum time of flight: 400
    Number of detector pixels: 12404
    Number of bins: 1
    Number of bins after rebinning: 100

.. categories::

.. sourcelink::
