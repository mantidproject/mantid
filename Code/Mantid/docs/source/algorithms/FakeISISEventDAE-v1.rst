.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Simulates ISIS event DAE. It runs continuously until canceled and
listens to port 10000 for connection. When connected starts sending
event packets.

Events are randomly distributed between the following 
 
- Periods
- Spectra
- time of flight between 10,000 and 20,000


Usage
-----

**Example:**

.. testcode:: exFakeISISEventDAE
    
    from threading import Thread

    def startFakeDAE():
        # This will generate 2000 events roughly every 20ms, so about 50,000 events/sec 
        # They will be randomly shared across the 100 spectra
        # and have a time of flight between 10,000 and 20,000
        try:
            FakeISISEventDAE(NPeriods=1,NSpectra=100,Rate=20,NEvents=1000)
        except RuntimeError:
            pass

    thread = Thread(target = startFakeDAE)
    thread.start()

    oldFacility = ConfigService.getFacility().name()
    ConfigService.setFacility("TEST_LIVE")

    # start a Live data listener updating every second, that rebins the data
    # and replaces the results each time with those of the last second.
    StartLiveData(Instrument='ISIS_Event', OutputWorkspace='wsOut', UpdateEvery=1,
        ProcessingAlgorithm='Rebin', ProcessingProperties='Params=10000,1000,20000;PreserveEvents=1', 
        AccumulationMethod='Replace', PreserveEvents=True)

    # give it a couple of seconds before stopping it
    Pause(2)

    # This will cancel both algorithms 
    # you can do the same in the GUI 
    # by clicking on the details button on the bottom right
    AlgorithmManager.newestInstanceOf("MonitorLiveData").cancel()
    AlgorithmManager.newestInstanceOf("FakeISISEventDAE").cancel()
    thread.join()
    ConfigService.setFacility(oldFacility)

    #get the ouput workspace
    wsOut = mtd["wsOut"]
    print "The workspace contains %i events" % wsOut.getNumberEvents()


Output: 


.. testoutput:: exFakeISISEventDAE
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

    The workspace contains ... events



.. categories::
