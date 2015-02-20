.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The StartLiveData algorithm launches a background job that monitors and
processes live data.

The background algorithm started is
:ref:`algm-MonitorLiveData`, which simply calls
:ref:`algm-LoadLiveData` at a fixed interval.

For details on the way to specify the data processing steps, see:
`LoadLiveData <LoadLiveData#Description>`__.

Live Plots
##########

Once live data monitoring has started, you can open a plot in
MantidPlot. For example, you can right-click a workspace and choose
"Plot Spectra".

As the data is acquired, this plot updates automatically.

Another way to start plots is to use `python MantidPlot
commands <MantidPlot:_Help#Python_Scripting_in_MantidPlot>`__. The
StartLiveData algorithm returns after the first chunk of data has been
loaded and processed. This makes it simple to write a script that will
open a live plot. For example:

.. code-block:: python

    StartLiveData(UpdateEvery='1.0',Instrument='FakeEventDataListener',
      ProcessingAlgorithm='Rebin',ProcessingProperties='Params=10e3,1000,60e3;PreserveEvents=1',
      OutputWorkspace='live')
    plotSpectrum('live', [0,1])

Run Transition Behavior
#######################

-  When the experimenter starts and stops a run, the Live Data Listener
   receives this as a signal.
-  The *RunTransitionBehavior* property specifies what to do at these
   run transitions.

   -  Restart: the accumulated data (from the previous run if a run has
      just ended or from the time between runs a if a run has just
      started) is discarded as soon as the next chunk of data arrives.
   -  Stop: live data monitoring ends. It will have to be restarted
      manually.
   -  Rename: the previous workspaces are renamed, and monitoring
      continues with cleared ones. The run number, if found, is used to
      rename the old workspaces.

      -  There is a check for available memory before renaming; if there
         is not enough memory, the old data is discarded.

-  Note that LiveData continues monitoring even if outside of a run
   (i.e. before a run begins you will still receive live data).

Multiple Live Data Sessions
###########################

It is possible to have multiple live data sessions running at the same
time. Simply call StartLiveData more than once, but make sure to specify
unique names for the *OutputWorkspace*.

Please note that you may be limited in how much simultaneous processing
you can do by your available memory and CPUs.

Usage
-----

**Example 1:**

.. testcode:: exStartLiveDataEvent

    from threading import Thread
    import time

    def startFakeDAE():
        # This will generate 2000 events roughly every 20ms, so about 50,000 events/sec
        # They will be randomly shared across the 100 spectra
        # and have a time of flight between 10,000 and 20,000
        try:
            FakeISISEventDAE(NPeriods=1,NSpectra=100,Rate=20,NEvents=1000)
        except RuntimeError:
            pass

    def captureLive():
        ConfigService.setFacility("TEST_LIVE")

        # start a Live data listener updating every second, that rebins the data
        # and replaces the results each time with those of the last second.
        StartLiveData(Instrument='ISIS_Event', OutputWorkspace='wsOut', UpdateEvery=1,
                      ProcessingAlgorithm='Rebin', ProcessingProperties='Params=10000,1000,20000;PreserveEvents=1',
                      AccumulationMethod='Add', PreserveEvents=True)

        # give it a couple of seconds before stopping it
        time.sleep(2)

        # This will cancel both algorithms
        # you can do the same in the GUI
        # by clicking on the details button on the bottom right
        AlgorithmManager.newestInstanceOf("MonitorLiveData").cancel()
        AlgorithmManager.newestInstanceOf("FakeISISEventDAE").cancel()
    #--------------------------------------------------------------------------------------------------

    oldFacility = ConfigService.getFacility().name()
    thread = Thread(target = startFakeDAE)
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
    print "The workspace contains %i events" % wsOut.getNumberEvents()

Output:

.. testoutput:: exStartLiveDataEvent
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

    The workspace contains ... events



**Example 2:**

.. testcode:: exStartLiveDataHisto

    from threading import Thread
    import time

    def startFakeDAE():
        # This will generate 5 periods of histogram data, 10 spectra in each period,
        # 100 bins in each spectrum
        try:
            FakeISISHistoDAE(NPeriods=5,NSpectra=10,NBins=100)
        except RuntimeError:
            pass

    def captureLive():
        ConfigService.setFacility("TEST_LIVE")

        # Start a Live data listener updating every second, 
        # that replaces the results each time with those of the last second.
        # Load only spectra 2,4, and 6 from periods 1 and 3
        StartLiveData(Instrument='ISIS_Histogram', OutputWorkspace='wsOut', UpdateEvery=1,
                            AccumulationMethod='Replace', PeriodList=[1,3],SpectraList=[2,4,6])

        # give it a couple of seconds before stopping it
        time.sleep(2)

        # This will cancel both algorithms
        # you can do the same in the GUI
        # by clicking on the details button on the bottom right
        AlgorithmManager.newestInstanceOf("MonitorLiveData").cancel()
        AlgorithmManager.newestInstanceOf("FakeISISHistoDAE").cancel()
    #--------------------------------------------------------------------------------------------------

    oldFacility = ConfigService.getFacility().name()
    thread = Thread(target = startFakeDAE)
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
    print "The workspace contains %i periods" % wsOut.getNumberOfEntries()
    print "Each period   contains %i spectra" % wsOut.getItem(0).getNumberHistograms()


Output:

.. testoutput:: exStartLiveDataHisto
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

    The workspace contains ... periods
    Each period   contains ... spectra


.. categories::
