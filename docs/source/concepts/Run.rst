.. _Run:

Run
===

What is it?
-----------

A Run holds data related to the properties of the experimental run, e.g.
good proton charge, total frames etc. It also holds all of the sample
log files as sets of time-series data. Currently used properties within
Mantid includes *run\_start*, which specified the date the data were
collected. Where an instrument has been modified over time, and multiple
:ref:`instrument definition files <InstrumentDefinitionFile>` have been
defined for it, this property is used to loads the IDF valid when the
data were collected.

What information is stored here?
--------------------------------

On loading experimental data there is a default set of properties that
are populated within the run. These are as follows:

ISIS (not including ISIS Muon data)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **run\_header** - The complete header for this run
-  **run\_title** - The run title
-  **run\_start** - Start date and time. Format: YYYY-MM-DD HH:MM:SS (+)
-  **run\_end** - End date and time. Format: YYYY-MM-DD HH:MM:SS (+)
-  **nspectra** - The number of spectra in the raw data file
-  **nchannels** - The number of time channels in the raw data
-  **nperiods** - The number of periods within the raw data
-  **dur** - The run duration
-  **durunits** - The units of the run duration, 1 = seconds
-  **dur\_freq** - Test interval for above
-  **dmp** - Dump interval
-  **dmp\_units** - The units (scaler) for above
-  **dmp\_freq** - Test interval for above
-  **freq** - 2\*\*k where source frequency = 50 / 2\*\*k
-  **gd\_prtn\_chrg** - Good proton charge (uA.hour)
-  '''tot\_prtn\_chrg\* '''- Total proton charge (uA.hour)
-  **goodfrm** - Good frames
-  '''rawfrm\* '''- Raw frames
-  **dur\_wanted** - Requested run duration (units as for "duration"
   above)
-  **dur\_secs** - Actual run duration in seconds
-  **mon\_sum1** - Monitor sum 1
-  **mon\_sum2** - Monitor sum 2
-  **mon\_sum3** - Monitor sum 3
-  **rb\_proposal** - The proposal number

ISIS Muon data
^^^^^^^^^^^^^^

-  **run\_title** - The run title
-  **run\_start** - Start date and time. Format: YYYY-MM-DD HH:MM:SS (+)
-  **run\_end** - End date and time. Format: YYYY-MM-DD HH:MM:SS (+)
-  **nspectra** - The number of spectra in the raw data file
-  **goodfrm** - Good frames
-  **dur\_secs** - Run duration in seconds
-  **run\_number** - Run number
-  **sample\_temp** - Temperature of the sample
-  **sample\_magn\_field** - Magnetic field of the sample

(+) or YYYY-MM-DDTHH:MM:SS (ISO 8601 format, see
`1 <http://en.wikipedia.org/wiki/ISO_8601>`__)



.. categories:: Concepts