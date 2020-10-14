.. _live_data_testing:

Live Data Testing
=================

.. contents::
   :local:

Introduction
------------
A video example of using multiple live streams and full direct elastic data reduction can be seen `here <http://files.mantidproject.org/videos/LiveData.htm>`__.

The rest of this document describes some examples you can run yourself using a fake instrument which simulates the arrival of new data. For all of these tests you will need to change your Facility to ``TEST_LIVE``, as described below. You can use MantidPlot or MantidWorkbench but it will need to be version 3.2.1 or above.

**Time required 20 - 30 minutes**

Set up the test facility
------------------------

Tn MantidPlot:

#. Open the ``Help`` menu and select ``First Time Setup``
#. Change the default facility to ``TEST_LIVE`` and click ``Set``

In MantidWorkbench:

#. Open the ``File`` menu and select ``Settings``
#. Change the facility to ``TEST_LIVE``

You can follow the same approach to set your facility back afterwards.

Starting a Fake Instrument
--------------------------

ISIS Event
##########

#. Run the ``FakeISISEventDAE`` Algorithm from the Algorithms toolbox
#. These settings will create an instrument with 100 spectra, transmitting 2000 events every 20 milliseconds in 1 period. The data is between 10,000 and 20,000 microseconds TOF:
   
   -  NPeriods = 1
   -  NSpectra = 100
   -  Rate = 20
   -  NEvents = 2000
   
#. To stop the fake instrument, open the Algorithm Details window, by clicking on the ``Running`` button (MantidPlot) or ``Details`` button (MantidWorkbench) at the bottom of the Algorithms Toolbox.  Then click ``Cancel`` next to ``FakeISISEventDAE``.

ISIS Histogram
##############

#. Run the ``FakeISISHistoDAE`` Algorithm from the Algorithms toolbox
#. These settings will create an instrument with 100 spectra, with 100 bins in 1 period:
   
   - NPeriods = 1
   - NSpectra = 100
   - NBins = 100

#. To stop the fake instrument, open the Algorithm Details window, by clicking on the ``Details`` button at the bottom of the Algorithms Toolbox.  Then click ``Cancel`` next to ``FakeISISHistoDAE``.

ADARA Fake Event
################

This approach reads from an SNS pre-nexus file to recreate realistic event data, however It is a little more fiddly to setup.

#. Find the file ``Mantid.user.properties``. Its location will be:
   
   - Windows: ``C:\MantidInstall\bin``
   - Mac or linux: ``~/.mantid`` (i.e. in a ``.mantid`` directory under your home directory)
   
#. To use the ``REF_L_32035_neutron_event.dat`` file (located in the ``TrainingCourseData`` folder), open ``Mantid.user.properties`` in your favorite text editor and add the following lines:

   ::
   
    fileeventdatalistener.filename=REF_L_32035_neutron_event.dat
    fileeventdatalistener.chunks=300
   
#. Start MantidPlot
#. Use the instrument ``ADARA_FileReader`` in the ``TEST_LIVE`` facility in the First Time Setup dialog (MantidPlot) or Settings dialog (MantidWorkbench).  There is no need to stop this fake instrument.

The data from this file comprises almost 50,000 events across 77,824 histograms, with TOF values between 6,000 and 23,000 microseconds.

Starting a live data session
----------------------------

#. Ensure that ``FakeISISHistoDAE`` and ``FakeISISEventDAE`` are running in the background to complete this example 
#. To open the ``StartLiveData`` interface you can either
   
   - click the drop down ``Load`` button in the Workspaces toolbox and select ``Live Data``
   - Run the ``StartLiveData`` algorithm from the Algorithms toolbox

Live Histogram Data
###################

Setup : ISIS Histogram

Note that for Histogram data the ``accumulationMethod`` needs to be set to ``Replace``, otherwise you will get a warning.

Enter the following settings in the StartLiveData interface:

- Instrument: ISIS_Histogram
- Start Time: now
- Update Every: 1 second
- Processing: No Processing
- Accumulation Method: Replace
- Post Processing: No Processing
- OutputWorkspace: live

Live event rebin using an algorithm
###################################

Setup : ADARA Fake Event

Enter the following settings in the StartLiveData interface:

- Instrument: ADARA_FakeEvent
- Start Time: now
- Update Every: 1 second
- Processing: Algorthim

  - Within the Processing step tab select the ``Rebin`` algorthm
  - In the algorithm properties set ``Params`` to ``40e3,1000,60e3``

- PreserveEvents: unticked
- Accumulation Method: Add
- OutputWorkspace: live

Live event pre- and post-processing
###################################

This uses rebin to select a region of time of flight, and then after 
the data is accumulated it uses SumSpectra to sum all of the data into a single spectrum.
When using post processing you have to give the accumulation workspace a name.

Setup : ADARA Fake Event

Enter the following settings in the StartLiveData interface:

- Instrument: ADARA_FakeEvent
- Start Time: now
- Update Every: 1 second
- Processing: Algorithm

  - Within the Processing step tab select the ``Rebin`` algorthm
  - In the algorthim properties set ``Params`` to ``40e3,500,60e3``
    
- PreserveEvents: Not Ticked
- Accumulation Method: Add
- Post Processing: Algorithm

  - Within the Post Processing step tab select the ``SumSpectra`` algorthm
    
- AccumulationWorkspace: accumulation 
- OutputWorkspace: live

Stopping a live data session
----------------------------

A live data session will run until it is cancelled or Mantid is closed.  To cancel a session:

#. To stop the live data session, open the ``Algorithm Details`` window, by clicking on the  button at the bottom of the Algorithms Toolbox
#. Find the Algorithm "MonitorLiveData" and click the "Cancel" button next to it.

