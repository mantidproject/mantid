.. _live_data_testing:

Live Data Testing
=================

.. contents::
   :local:

Introduction
------------
A video example of using multiple live streams and full direct elastic data reduction can be seen `here <../../_static/videos/LiveData.htm>`__.

The rest of this document describes some examples you can run yourself using a fake instrument which simulates the arrival of new data.
For all of these tests you will need to change your Facility to ``TEST_LIVE``, as described below.
You can use MantidWorkbench.

**Time required 20 - 30 minutes**

Set up the test facility
------------------------

In MantidWorkbench:

#. Open the ``File`` menu and select ``Settings``
#. Change the facility to ``TEST_LIVE``

You can follow the same approach to set your facility back afterwards.

Starting a Fake Instrument
--------------------------

.. _ISIS Event:

ISIS Event
##########

#. Run the ``FakeISISEventDAE`` algorithm from the Algorithms toolbox
#. The following settings will create an instrument with 100 spectra, transmitting 2000 events every 20 milliseconds in 1 period. The data is between 10,000 and 20,000 microseconds TOF:

   -  NPeriods = 1
   -  NSpectra = 100
   -  Rate = 20
   -  NEvents = 2000

#. To stop the fake instrument, open the Algorithm Details window, by clicking on the ``Details`` button (MantidWorkbench) at the bottom of the Algorithms Toolbox.  Then click ``Cancel`` next to ``FakeISISEventDAE``.

Running this algorithm will produce "``FakeISISEventDAE started``" in the Messages panel, but will not make any other data appear yet.

.. _ISIS Histogram:

ISIS Histogram
##############

#. Run the ``FakeISISHistoDAE`` Algorithm from the Algorithms toolbox
#. The following settings will create an instrument with 100 spectra, with 100 bins in 1 period:

   - NPeriods = 1
   - NSpectra = 100
   - NBins = 100

#. To stop the fake instrument, open the Algorithm Details window, by clicking on the ``Details`` button at the bottom of the Algorithms Toolbox.  Then click ``Cancel`` next to ``FakeISISHistoDAE``.

Running this algorithm will produce "``FakeISISHistoDAE started``" in the Messages panel, but will not make any other data appear yet.

.. _ADARA Fake Event:

ADARA Fake Event
################

This approach reads from an SNS pre-nexus file or an event-nexus file to recreate realistic event data, however It is a little more fiddly to setup.

#. Find the file ``Mantid.user.properties``. Its location will be:

   - Windows: ``C:\MantidInstall\bin``
   - Mac or linux: ``~/.mantid`` (i.e. in a ``.mantid`` directory under your home directory)

#. To use a pre-nexus file, use the ``REF_L_32035_neutron_event.dat`` file (located in the ``TrainingCourseData`` folder), open the ``Mantid.user.properties`` file in your favorite text editor and add the following lines:

   ::

    fileeventdatalistener.filename=REF_L_32035_neutron_event.dat
    fileeventdatalistener.chunks=300

   A event nexus file can also be used instead, for example:

   ::

    fileeventdatalistener.filename=EQSANS_6071_event.nxs

   For MantidWorkbench to find these files, they must be in a directory listed under "Data Search Directories" in the "Manage User Directories" menu. Chunking for files can be determined by using the :ref:`DetermineChunking <algm-DetermineChunking>` algorithm.

#. Start MantidWorkbench
#. Use the instrument ``ADARA_FileReader`` in the ``TEST_LIVE`` facility in the Settings dialog (MantidWorkbench).  There is no need to stop this fake instrument.

The data from this file comprises almost 50,000 events across 77,824 histograms, with TOF values between 6,000 and 23,000 microseconds.

Starting a live data session
----------------------------

#. Ensure that ``FakeISISHistoDAE`` and ``FakeISISEventDAE`` are running in the background to complete this example
#. To open the ``StartLiveData`` interface you can either

   - Click the drop down ``Load`` button in the Workspaces toolbox and select ``Live Data``
   - Run the ``StartLiveData`` algorithm from the Algorithms toolbox

Live Event Data
###############

Setup: :ref:`ISIS Event`

Enter the following settings in the StartLiveData interface:

- Instrument: ISIS_Event
- Start Time: now
- Update Every: 1 second
- Processing: No Processing
- Accumulation Method: Replace
- Post Processing: No Processing
- OutputWorkspace: live

Click "Run".

You should see successive messages in MantidWorkbench of the form ``Loading live data chunk x at xx:xx:xx``.

Live Histogram Data
###################

Setup : :ref:`ISIS Histogram`

Note that for Histogram data the ``accumulationMethod`` needs to be set to ``Replace``, otherwise you will get a warning.

Enter the following settings in the StartLiveData interface:

- Instrument: ISIS_Histogram
- Start Time: now
- Update Every: 1 second
- Processing: No Processing
- Accumulation Method: Replace
- Post Processing: No Processing
- OutputWorkspace: live

Click "Run".

You should see successive messages in MantidWorkbench of the form ``Loading live data chunk x at xx:xx:xx``.

Live event rebin using an algorithm
###################################

Setup : :ref:`ADARA Fake Event`

Enter the following settings in the StartLiveData interface:

- Instrument: ADARA_FakeEvent
- Start Time: now
- Update Every: 1 second
- Processing: Algorthim

  - Within the Processing step tab select the ``Rebin`` algorthm
  - In the algorithm properties set ``Params`` to ``40e3,1000,60e3``
  - ``PreserveEvents`` should be unticked

- Accumulation Method: Add
- OutputWorkspace: live

Click "Run".

You should see successive messages in MantidWorkbench of the form ``Loading live data chunk x at xx:xx:xx``.

Double-click on the ``live`` workspace, then select ``Plot All``. You should see a histogram with two lines automatically updating as "new" data appears.

Live event pre- and post-processing
###################################

This uses Rebin to select a region of time of flight, and then after
the data is accumulated it uses SumSpectra to sum all of the data into a single spectrum.
When using post processing you have to give the accumulation workspace a name.

Setup : :ref:`ADARA Fake Event`

Enter the following settings in the StartLiveData interface:

- Instrument: ADARA_FakeEvent
- Start Time: now
- Update Every: 1 second
- Processing: Algorithm

  - Within the Processing step tab select the ``Rebin`` algorthm
  - In the algorthim properties set ``Params`` to ``40e3,500,60e3``
  - ``PreserveEvents`` should be unticked

- Accumulation Method: Add
- Post Processing: Algorithm

  - Within the Post Processing step tab select the ``SumSpectra`` algorthm
  - The options can be left as their defaults

- AccumulationWorkspace: accumulation
- OutputWorkspace: live

Click "Run".

You should see successive messages in MantidWorkbench of the form ``Loading live data chunk x at xx:xx:xx``.

Double-click on the ``live`` workspace. You should see a histogram with one line automatically updating as "new" data appears.
Double-click on the ``accumulation`` workspace, then select ``Plot All``. You should see a histogram with two lines automatically updating as "new" data appears.

Stopping a live data session
----------------------------

A live data session will run until it is cancelled or Mantid is closed.  To cancel a session:

#. To stop the live data session, open the ``Algorithm Details`` window, by clicking on the  button at the bottom of the Algorithms Toolbox
#. Find the Algorithm "MonitorLiveData" and click the "Cancel" button next to it.
