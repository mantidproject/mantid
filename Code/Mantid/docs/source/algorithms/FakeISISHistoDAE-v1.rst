.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Simulates ISIS histogram DAE. It runs continuously until canceled and
listens to port 6789 for ISIS DAE commands.

Data is generated starting at 10000 microseconds Time of flight, and each bin requested covers 100 microseconds.

Usage
-----

**Example:**

.. testcode:: exFakeISISHistoDAE
    
    from threading import Thread

    def startFakeHistoDAE():
        # This will generate 2000 events roughly every 20ms, so about 50,000 events/sec 
        # They will be randomly shared across the 100 spectra
        # and have a time of flight between 100 and 200
        try:
            FakeISISHistoDAE(NPeriods=1,NSpectra=100,nBins=100)
        except RuntimeError:
            pass

    thread = Thread(target = startFakeHistoDAE)
    thread.start()

    oldFacility = ConfigService.getFacility().name()
    ConfigService.setFacility("TEST_LIVE")

    # start a Live data listener updating every second, that rebins the data
    # and replaces the results each time with those of the last second.
    StartLiveData(Instrument='ISIS_Histogram', OutputWorkspace='wsOut', UpdateEvery=1,
        AccumulationMethod='Replace')

    # give it a couple of seconds before stopping it
    Pause(2)

    # This will cancel both algorithms 
    # you can do the same in the GUI 
    # by clicking on the details button on the bottom right
    AlgorithmManager.newestInstanceOf("MonitorLiveData").cancel()
    AlgorithmManager.newestInstanceOf("FakeISISHistoDAE").cancel()
    thread.join()
    ConfigService.setFacility(oldFacility)

    #get the ouput workspace
    wsOut = mtd["wsOut"]
    print "The workspace contains %i histograms" % wsOut.getNumberHistograms()


Output: 


.. testoutput:: exFakeISISHistoDAE
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

    The workspace contains 10 histograms


.. categories::
