.. _mantid.api.Run:

=====
 Run
=====

This is a Python binding to the C++ class Mantid::API::Run.

A Run holds data related to the properties of the experimental run, e.g.
good proton charge, total frames etc. It also holds all of the sample
log files as sets of time-series data. Currently used properties within
Mantid includes *run_start*, which specified the date the data were
collected. Where an instrument has been modified over time, and multiple
:ref:`instrument definition files <InstrumentDefinitionFile>` have been
defined for it, this property is used to loads the IDF valid when the
data were collected.


Working with Run object in Python
---------------------------------

Getting the Run Object from a Workspace
#######################################

.. testsetup:: WorkspaceRun

  ws = CreateSampleWorkspace()

.. testcode:: WorkspaceRun

  run = ws.getRun()

Run Properties
##############

.. testsetup:: RunPropertiestest

  ws = Load("MAR11060")

.. testcode:: RunPropertiestest

  from mantid.kernel import DateAndTime
  run = ws.getRun()

  # Set the start and end time of a run
  run.setStartAndEndTime(DateAndTime("2015-01-27T11:00:00"),
  DateAndTime("2015-01-27T11:57:51"))

  # Get the start and end time of a run
  print(run.startTime())
  print(run.endTime())

  # Get the total good proton charge
  print(run.getProtonCharge())

.. testoutput:: RunPropertiestest
  :hide:
  :options: +ELLIPSIS,+NORMALIZE_WHITESPACE

  2015-01-27T11:00:00
  2015-01-27T11:57:51
  121...

Accessing Properties
####################

Listing all properties
^^^^^^^^^^^^^^^^^^^^^^

.. testcode:: RunListPropertiestest

  ws = Load("MAR11060")

  run = ws.getRun()

  # Get a list of the property names
  print(run.keys())

  # Loop over all of the Properties
  for prop in run.getProperties():
      print("{0} {1}".format(prop.name, prop.value))

.. testoutput:: RunListPropertiestest
  :hide:
  :options: +ELLIPSIS,+NORMALIZE_WHITESPACE

  ['run_header', ... 'run_title']
  run_header MAR 11060                      Vanadium white beam      23-JUN-2005  10:18:46    121.5
  ...
  run_title Vanadium white beam                              jaws=50x50 nim=50 dsc=0

Getting a specific property
^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. testcode:: RunGetPropertytest

  ws = CreateSampleWorkspace()

  run = ws.getRun()

  # Check if property exists
  print("Is runstart present: {0}".format(("run_start" in run.keys())))
  # or
  print("Is runstart present: {0}".format(run.hasProperty("run_start")))

  #get the Property
  runStart = run.getProperty("run_start")
  print("Property name: " + runStart.name)
  print("Property value: " + runStart.value)

.. testoutput:: RunGetPropertytest
  :hide:
  :options: +ELLIPSIS,+NORMALIZE_WHITESPACE

  Is runstart present: True
  Is runstart present: True
  Property name: run_start
  Property value: 2010-01-01T00:00:00

The Goniometer
##############

If the instrument contains a Goniometer it can be accessed from the run object.

.. testcode:: GetGoniometertest

  wg=CreateSingleValuedWorkspace()
  AddSampleLog(wg,"Motor1","45.","Number")
  SetGoniometer(wg,Axis0="Motor1,0,1,0,1",Axis1="5,0,1,0,1")

  print("Goniometer angles:  {}".format(wg.getRun().getGoniometer().getEulerAngles('YZY')))

.. testoutput:: GetGoniometertest
  :hide:
  :options: +NORMALIZE_WHITESPACE

  Goniometer angles:  [50,0,0]

Multiple goniometers can be accessed from the run object by

.. testcode:: GetGoniometertest2

  wg=CreateSingleValuedWorkspace()
  AddTimeSeriesLog(wg, Name="Motor1", Time="2010-01-01T00:00:00", Value=0)
  AddTimeSeriesLog(wg, Name="Motor1", Time="2010-01-01T00:01:00", Value=45)
  AddTimeSeriesLog(wg, Name="Motor1", Time="2010-01-01T00:02:00", Value=90)
  SetGoniometer(wg,Axis0="Motor1,0,1,0,1",Average=False)

  print("Number of goniometers =", wg.getRun().getNumGoniometers())

  for i in range(wg.getRun().getNumGoniometers()):
      print("Goniometer angles:  {}".format(wg.getRun().getGoniometer(i).getEulerAngles('YZY')))

.. testoutput:: GetGoniometertest2
  :hide:
  :options: +NORMALIZE_WHITESPACE

  Number of goniometers = 3
  Goniometer angles:  [0,0,0]
  Goniometer angles:  [45,0,0]
  Goniometer angles:  [90,0,0]

What information is stored here?
---------------------------------

On loading experimental data there is a default set of properties that
are populated within the run. These are as follows:

ISIS (not including ISIS Muon data)
###################################

-  **run_header** - The complete header for this run
-  **run_title** - The run title
-  **run_start** - Start date and time. Format: YYYY-MM-DD HH:MM:SS (+)
-  **run_end** - End date and time. Format: YYYY-MM-DD HH:MM:SS (+)
-  **nspectra** - The number of spectra in the raw data file
-  **nchannels** - The number of time channels in the raw data
-  **nperiods** - The number of periods within the raw data
-  **dur** - The run duration
-  **durunits** - The units of the run duration, 1 = seconds
-  **dur_freq** - Test interval for above
-  **dmp** - Dump interval
-  **dmp_units** - The units (scaler) for above
-  **dmp_freq** - Test interval for above
-  **freq** - 2\*\*k where source frequency = 50 / 2\*\*k
-  **gd_prtn_chrg** - Good proton charge (uA.hour)
-  **tot_prtn_chrg** - Total proton charge (uA.hour)
-  **goodfrm** - Good frames
-  **rawfrm** - Raw frames
-  **dur_wanted** - Requested run duration (units as for "duration" above)
-  **dur_secs** - Actual run duration in seconds
-  **mon_sum1** - Monitor sum 1
-  **mon_sum2** - Monitor sum 2
-  **mon_sum3** - Monitor sum 3
-  **rb_proposal** - The proposal number

.. _RunInfoOnISISMuonData:

ISIS Muon data
##############

-  **run_title** - The run title
-  **run_start** - Start date and time. Format: YYYY-MM-DD HH:MM:SS (+)
-  **run_end** - End date and time. Format: YYYY-MM-DD HH:MM:SS (+)
-  **nspectra** - The number of spectra in the raw data file
-  **goodfrm** - Good frames
-  **dur_secs** - Run duration in seconds
-  **run_number** - Run number
-  **sample_temp** - Temperature of the sample
-  **sample_magn_field** - Magnetic field of the sample

(+) or YYYY-MM-DDTHH:MM:SS (ISO 8601 format, see
`1 <http://en.wikipedia.org/wiki/ISO_8601>`__)


Reference
---------

.. module:`mantid.api`

.. autoclass:: mantid.api.Run
    :members:
    :undoc-members:
    :inherited-members:
