.. _train-MBC_Live_Data_Simple_Examples:


=========================
Live Data Simple Examples
=========================

Video example
=============

An example of using multiple live streams and full direct elastic data
reduction can be seen at
`http://download.mantidproject.org/videos/LiveData.htm <http://files.mantidproject.org/videos/LiveData.htm>`__.

Do it yourself examples
=======================

For all of these you will need to change your Facility to TEST_LIVE.
You will need to be using Mantid version 3.2.1 or above.

#. Open the "Help" menu and select "First Time Setup"
#. Change the default facility to TEST_LIVE and click "Set"

You can follow the same approach to set your facility back afterwards.

Starting a Fake Instrument
--------------------------

ISIS Event
~~~~~~~~~~

#. Run the :ref:`FakeISISEventDAE <algm-FakeISISEventDAE>` Algorithm from the Algorithms toolbox
#. These settings will create an instrument with 100 spectra,
   transmitting 2000 events every 20 milliseconds in 1 period. The data
   is between 10,000 and 20,000 microseconds TOF.

   #. NPeriods = 1
   #. NSpectra = 100
   #. Rate = 20
   #. NEvents = 2000

#. To stop the fake instrument, open the Algorithm Details window, by
   clicking on the "Details" button at the bottom of the Algorithms
   Toolbox. Then click "Cancel" next to :ref:`FakeISISEventDAE <algm-FakeISISEventDAE>`.

ISIS Histogram
~~~~~~~~~~~~~~

#. Run the :ref:`FakeISISHistoDAE <algm-FakeISISHistoDAE>` Algorithm from the Algorithms toolbox
#. These settings will create an instrument with 100 spectra, with 100
   bins in 1 period.

   #. NPeriods = 1
   #. NSpectra = 100
   #. NBins = 100

#. To stop the fake instrument, open the Algorithm Details window, by
   clicking on the "Details" button at the bottom of the Algorithms
   Toolbox. Then click "Cancel" next to :ref:`FakeISISHistoDAE <algm-FakeISISHistoDAE>`.

ADARA Fake Event
~~~~~~~~~~~~~~~~

No setup is needed, Just use the instrument "ADARA_FakeEvent" in the
"TEST_LIVE" facility. Equally there is no need to stop this fake
instrument.

The events are sent as coming from 2 spectra, and 200 events/second,
with a time of flight between 40,000 and 60,000.

ADARA File Reader
~~~~~~~~~~~~~~~~~

This approach reads from an SNS pre-nexus file to recreate realistic
event data, however It is a little more fiddly to setup.

#. Find the file "mantid.user.properties" it's location will vary with
   your operating system.

   -  Windows: c:\\MantidInstall\\bin
   -  Mac or linux: ~/.mantid (in a .mantid directory under your home
      directory)

#. Edit the file in your favorite text editor and add the following
   lines::

	fileeventdatalistener.filename=REF_L_32035_neutron_event.dat
	fileeventdatalistener.chunks=300

#. Start MantidPlot
#. Use the instrument "ADARA_FileReader" in the "TEST_LIVE" facility.
   There is no need to stop this fake instrument.

The data from this file comprises almost 50,000 events across 77,824
histograms, with TOF values between 6,000 and 23,000 microseconds.

Starting a live data session
----------------------------

#. To open the :ref:`StartLiveData <algm-StartLiveData>` interface you can either

   -  click the drop down ":ref:`Load <algm-Load>`" button in the Workspaces toolbox and
      select "Live Data"
   -  Run the :ref:`StartLiveData <algm-StartLiveData>` algorithm from the Algorithms toolbox

Stopping a live data session
----------------------------

A live data session will run until it is cancelled or Mantid is closed.
To cancel a session:

#. To stop the live data session, open the Algorithm Details window, by
   clicking on the "Details" button at the bottom of the Algorithms
   Toolbox
#. Find the Algorithm "MonitorLiveData" and click the "Cancel" button
   next to it.

Live Histogram Data
~~~~~~~~~~~~~~~~~~~

Setup : ISIS Histogram For Histogram data the accumulationMethod needs
to be set to Replace, you will get a warning otherwise.

#. Open the :ref:`StartLiveData <algm-StartLiveData>` interface
#. Instrument: ISIS_Histogram
#. Start Time: now
#. Update Every: 1 second
#. Processing: No Processing
#. Accumulation Method: Replace
#. Post Processing: No Processing
#. OutputWorkspace: live

Live Event Rebin using an algorithm
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Setup : ADARA Fake Event

#. Open the :ref:`StartLiveData <algm-StartLiveData>` interface
#. Instrument: ADARA_FakeEvent
#. Start Time: now
#. Update Every: 1 second
#. Processing: Algorthim

   #. Within the Processing step tab select the :ref:`Rebin <algm-Rebin>` algorithm
   #. In the algorithm properties set **Params** to 40e3,1000,60e3

#. PreserveEvents: unticked
#. Accumulation Method: Add
#. OutputWorkspace: live

Live Event Pre and post processing
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This uses rebin to select a region of time of flight, and then after the
data is accumulated it uses :ref:`SumSpectra <algm-SumSpectra>` to sum all of the data into a
single spectrum. When using post processing you have to give the
accumulation workspace a name.

Setup : ADARA Fake Event

#. Open the :ref:`StartLiveData <algm-StartLiveData>` interface
#. Instrument: ADARA_FakeEvent
#. Start Time: now
#. Update Every: 1 second
#. Processing: Algorithm

   #. Within the Processing step tab select the :ref:`Rebin <algm-Rebin>` algorithm
   #. In the algorthim properties set **Params** to 40e3,500,60e3

#. PreserveEvents: Not Ticked
#. Accumulation Method: Add
#. Post Processing: Algorithm

   #. Within the Post Processing step tab select the :ref:`SumSpectra <algm-SumSpectra>`
      algorithm

#. AccumulationWorkspace: accumulation
#. OutputWorkspace: live

Examples using the ADARA File Reader
------------------------------------

For these you will nedd to have performed the steps in the ADARA File
Reader setup above.

Rebinning the data & plotting
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#. Open the :ref:`StartLiveData <algm-StartLiveData>` interface
#. Start Time: now
#. Update Every: 1 second
#. Processing: Algorithm

   #. Within the Processing step tab select the :ref:`Rebin <algm-Rebin>` algorithm
   #. In the algorthim properties set **Params** to 6e3,200,23e3

#. PreserveEvents: Not Ticked
#. Accumulation Method: Add
#. Post Processing: None
#. OutputWorkspace: live

Take a look at the instrument view, and try plotting spectra 38020. Both
views should update as more data comes in.

You could repeat this with the accumulation method set to Replace. That
would give you just the events as seen over the last second.

Listen and post process with python script
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#. Open the :ref:`StartLiveData <algm-StartLiveData>` interface
#. Start Time: now
#. Update Every: 1 second
#. Processing: Algorithm

   #. Within the Processing step tab select the :ref:`SumSpectra <algm-SumSpectra>` algorithm

#. PreserveEvents: Not Ticked
#. Accumulation Method: Add
#. Post Processing: Script

   #. Within the Post Processing step tab use the script below

#. OutputWorkspace: live

.. code:: python

    from datetime import datetime

    #Now get the data, read the first spectra
    spectra=input.readY(0)
    #extract the first value from the array
    count=spectra[0]
    #output it as a log message
    logger.notice("Total counts so far " + str(count))

    #if my output workspace has not been created yet, create it.
    if not mtd.doesExist(output):
        table=:ref:`CreateEmptyTableWorkspace <algm-CreateEmptyTableWorkspace>`(OutputWorkspace=output)
        table.setTitle("Event Rate History")
        table.addColumn("str", "Time")
        table.addColumn("int", "Events")

    table = mtd[output]

    table.addRow([datetime.now().isoformat(), int(count)])


This will log the current total of events, but also creates a table
workspace with the history of that value.



