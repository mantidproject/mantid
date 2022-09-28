.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------


This algorithm loads a DNS mesytec psd listmode file into an :ref:`EventWorkspace <EventWorkspace>`, it does not load the instrument definition.

**Output**

    :ref:`EventWorkspace <EventWorkspace>` with the X-axis in the Time-of-flight units.

**ChopperChannel**
The chopper input channel as set in mesydaq gui.
It can be set to integers between 1 and 4. At DNS default is 2.


**NumberOfTubes**
The number of tubes of the detector, each tube has 1024 pixels. The full detector at DNS has 128 tubes. It can be set to integers between 1 and 128.


**DiscardPreChopperEvents**:
If set to *true* events with a timestamp before the first chopper timestamp are ignored, since they have no valid TOF.
If the dataset is elastic they can be included, since only the position and not the TOF is used.


**SetBinBoundary**:
If set to *true* the bin boundaries of the spectrum histrograms are set to include all events in one bin (0 to maximum TOF).
It can be turned off for faster loading, then the histograms are empty, with both boundaries close to 0.
The binning can then later be reproduced by applying  :ref:`Rebin <algm-Rebin>` with the paraemter *Params='0, {0}, {0}'.format(eventWS.getTofMax())*.


Restrictions
------------

- This algorithm only supports the *DNS* instrument.

- This algorithm loads raw TOF data, without normalization and without considering the detectorbank position.


Usage
-----

**Example 1 - Load TOF PSD data:**

.. testcode:: ExLoadDNSEvent

    eventWS = LoadDNSEvent(InputFile="DNS_psd_150c_first_tube.mdat", ChopperChannel='2', NumberOfTubes='1')
    print("Number of events: {}".format(eventWS.getNumberEvents()))
    print("Maximum time of flight: {}".format(eventWS.getTofMax()))
    print("Number of detector pixels: {}".format(eventWS.getNumberHistograms()))
    print("Number of bins: {}".format(eventWS.blocksize()))

    # rebin spectra:
    test2 = Rebin(InputWorkspace='eventWS', Params='0, 100, {0}'.format(eventWS.getTofMax()))
    print("Number of bins after rebinning: {}".format(test2.blocksize()))


**Output:**

.. testoutput:: ExLoadDNSEvent
   :options: +NORMALIZE_WHITESPACE

    Number of events: 184
    Maximum time of flight: 6641.4
    Number of detector pixels: 1024
    Number of bins: 1
    Number of bins after rebinning: 67

.. categories::

.. sourcelink::
