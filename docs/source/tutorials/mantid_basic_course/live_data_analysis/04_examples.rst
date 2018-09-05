.. _04_examples:

=========
 Examples 
=========

Do it yourself examples
=======================

For all of these you will need to change your Facility to TEST_LIVE. You
will need to be using Mantid version 3.2.1 or above.

#. Open the "Help" menu and select "First Time Setup"
#. Change the default facility to TEST_LIVE and click "Set"

You can follow the same approach to set your facility back afterwards.

Starting a Fake Instrument
--------------------------

ISIS Event
~~~~~~~~~~

#. Run the FakeISISEventDAE Algorithm from the Algorithms toolbox
#. These settings will create an instrument with 100 spectra,
   transmitting 2000 events every 20 milliseconds in 1 period. The data
   is between 10,000 and 20,000 microseconds TOF.

   #. NPeriods = 1
   #. NSpectra = 100
   #. Rate = 20
   #. NEvents = 2000

#. To stop the fake instrument, open the Algorithm Details window, by
   clicking on the "Running" button at the bottom of the Algorithms
   Toolbox. Then click "Cancel" next to FakeISISEventDAE.

ISIS Histogram
~~~~~~~~~~~~~~

#. Run the FakeISISHistoDAE Algorithm from the Algorithms toolbox
#. These settings will create an instrument with 100 spectra, with 100
   bins in 1 period.

   #. NPeriods = 1
   #. NSpectra = 100
   #. NBins = 100

#. To stop the fake instrument, open the Algorithm Details window, by
   clicking on the "Details" button at the bottom of the Algorithms
   Toolbox. Then click "Cancel" next to FakeISISHistoDAE.

ADARA Fake Event
~~~~~~~~~~~~~~~~

This approach reads from an SNS pre-nexus file to recreate realistic
event data, however It is a little more fiddly to setup.

#. Find the file "mantid.user.properties" it's location will vary with
   your operating system.

   -  Windows: c:\MantidInstall\bin
   -  Mac or linux: ~/.mantid (in a .,mantid directory under your home
      directory)

#. To use the "REF_L_32035_neutron_event.dat" file (located in the
   "TrainingCourseData" folder), open Mantid.user.properties in your
   favorite text editor and add the following lines:

| ``fileeventdatalistener.filename=REF_L_32035_neutron_event.dat``
| ``fileeventdatalistener.chunks=300``

#. Start MantidPlot
#. Use the instrument "ADARA_FileReader" in the "TEST_LIVE" facility in
   the First Time Setup. There is no need to stop this fake instrument.

The data from this file comprises almost 50,000 events across 77,824
histograms, with TOF values between 6,000 and 23,000 microseconds.

Starting a live data session
----------------------------

#. Ensure that FakeISISHistoDAE and FakeISISEventDAE are running in the
   background to complete this example
#. To open the StartLiveData interface you can either

   -  click the drop down "Load" button in the Workspaces toolbox and
      select "Live Data"
   -  Run the StartLiveData algorithm from the Algorithms toolbox

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

#. Open the StartLiveData interface
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

#. Open the StartLiveData interface
#. Instrument: ADARA_FakeEvent
#. Start Time: now
#. Update Every: 1 second
#. Processing: Algorthim

   #. Within the Processing step tab select the **Rebin** algorthm
   #. In the algorithm properties set **Params** to 40e3,1000,60e3

#. PreserveEvents: unticked
#. Accumulation Method: Add
#. OutputWorkspace: live

Live Event Pre and post processing
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This uses rebin to select a region of time of flight, and then after the
data is accumulated it uses SumSpectra to sum all of the data into a
single spectrum. When using post processing you have to give the
accumulation workspace a name.

Setup : ADARA Fake Event

#. Open the StartLiveData interface
#. Instrument: ADARA_FakeEvent
#. Start Time: now
#. Update Every: 1 second
#. Processing: Algorithm

   #. Within the Processing step tab select the **Rebin** algorthm
   #. In the algorthim properties set **Params** to 40e3,500,60e3

#. PreserveEvents: Not Ticked
#. Accumulation Method: Add
#. Post Processing: Algorithm

   #. Within the Post Processing step tab select the **SumSpectra**
      algorthm

#. AccumulationWorkspace: accumulation
#. OutputWorkspace: live

.. raw:: mediawiki

   {{SlideNavigationLinks|MBC_Live_Data_User_Interface|Mantid_Basic_Course|MBC_MDWorkspaces}}
