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
    import time

    def startFakeHistoDAE():
        # This will generate 100 spectra with 100 bins
        try:
            FakeISISHistoDAE(NPeriods=1,NSpectra=100,nBins=100)
        except RuntimeError:
            pass

    def captureLive():
        ConfigService.setFacility("TEST_LIVE")

        # start a Live data listener updating every second, that rebins the data
        # and replaces the results each time with those of the last second.
        StartLiveData(Instrument='ISIS_Histogram', OutputWorkspace='wsOut', UpdateEvery=1,
                      AccumulationMethod='Replace')

        # give it a couple of seconds before stopping it
        time.sleep(2)

        # This will cancel both algorithms
        # you can do the same in the GUI
        # by clicking on the details button on the bottom right
        AlgorithmManager.newestInstanceOf("MonitorLiveData").cancel()
        AlgorithmManager.newestInstanceOf("FakeISISHistoDAE").cancel()
        time.sleep(1) # give them time to cancel
    #--------------------------------------------------------------------------------------------------

    oldFacility = ConfigService.getFacility().name()
    thread = Thread(target = startFakeHistoDAE)
    thread.start()
    time.sleep(2) # give it a small amount of time to get ready
    if not thread.is_alive():
        raise RuntimeError("Unable to start FakeDAE")

    try:
        captureLive()
    except Exception, exc:
        print "Error occurred starting live data"
    finally:
        thread.join() # this must get hit

    # put back the facility
    ConfigService.setFacility(oldFacility)

    #get the ouput workspace
    wsOut = mtd["wsOut"]
    print "The workspace contains %i histograms" % wsOut.getNumberHistograms()

Output:

.. testoutput:: exFakeISISHistoDAE
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

    The workspace contains 100 histograms

.. categories::
