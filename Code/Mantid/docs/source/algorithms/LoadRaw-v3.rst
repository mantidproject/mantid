.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The LoadRaw algorithm stores data from the `RAW <http://www.mantidproject.org/Raw File>`__ file in a
`Workspace2D <http://www.mantidproject.org/Workspace2D>`__, which will naturally contain histogram
data with each spectrum going into a separate histogram. The time bin
boundaries (X values) will be common to all histograms and will have
their `units <http://www.mantidproject.org/Units>`__ set to time-of-flight. The Y values will contain
the counts and will be unit-less (i.e. no division by bin width or
normalisation of any kind). The errors, currently assumed Gaussian, will
be set to be the square root of the number of counts in the bin.

Optional properties
###################

If only a portion of the data in the `RAW <http://www.mantidproject.org/Raw File>`__ file is
required, then the optional 'spectrum' properties can be set before
execution of the algorithm. Prior to loading of the data the values
provided are checked and the algorithm will fail if they are found to be
outside the limits of the dataset.

Multiperiod data
################

If the RAW file contains multiple periods of data this will be detected
and the different periods will be output as separate workspaces, which
after the first one will have the period number appended (e.g.
OutputWorkspace\_period). Each workspace will share the same
`Instrument <http://www.mantidproject.org/Instrument>`__, SpectraToDetectorMap and
`Sample <http://www.mantidproject.org/Sample>`__ objects. If the optional 'spectrum' properties are
set for a multiperiod dataset, then they will ignored.

If PeriodList property isn't empty then only periods listed there will be
loaded.

Subalgorithms used
##################

LoadRaw runs the following algorithms as child algorithms to populate
aspects of the output `Workspace <http://www.mantidproject.org/Workspace>`__:

-  :ref:`algm-LoadInstrument` - Looks for an instrument
   definition file named XXX\_Definition.xml, where XXX is the 3 letter
   instrument prefix on the RAW filename, in the directory specified by
   the "instrumentDefinition.directory" property given in the config
   file (or, if not provided, in the relative path ../Instrument/). If
   the instrument definition file is not found then the
   :ref:`algm-LoadInstrumentFromRaw` algorithm will be
   run instead.
-  :ref:`algm-LoadMappingTable` - To build up the mapping
   between the spectrum numbers and the Detectors of the attached
   `Instrument <http://www.mantidproject.org/Instrument>`__.
-  :ref:`algm-LoadLog` - Will look for any log files in the same
   directory as the RAW file and load their data into the workspace's
   `Sample <http://www.mantidproject.org/Sample>`__ object.

Previous Versions
-----------------

LoadRaw version 1 and 2 are no longer available in Mantid. Version 3 has
been validated and in active use for several years, if you really need a
previous version of this algorithm you will need to use an earlier
version of Mantid.

Usage
-----

.. include:: ../usagedata-note.txt

Example 1: using defaults
#########################

.. testcode:: ExLoadDataDefaults

  # Load a RAW file using all defaults
  ws = Load('HRP39180.RAW')
  print 'Workspace has',ws.getNumberHistograms(),'spectra'
  # Get a detector in histogram 1024 of the workspace
  detid = ws.getSpectrum(1024).getDetectorIDs()[0]
  print 'Is detector',detid,'a monitor?',ws.getInstrument().getDetector(detid).isMonitor()

  # Get the Run object
  run = ws.run()
  print 'Workspace contains',len(run.keys()),'logs'
  print 'Workspace has property INT1:',run.hasProperty('INT1')

Output
^^^^^^

.. testoutput:: ExLoadDataDefaults

  Workspace has 2890 spectra
  Is detector 1001 a monitor? True
  Workspace contains 35 logs
  Workspace has property INT1: True


Example 2: load without the logs
################################

.. testcode:: ExLoadDataNoLogs

  # Load a RAW file without the logs
  ws = Load('HRP39180.RAW',LoadLogFiles=False)
  print 'Workspace has',ws.getNumberHistograms(),'spectra'
  # Get a detector in histogram 1024 of the workspace
  detid = ws.getSpectrum(1024).getDetectorIDs()[0]
  print 'Is detector',detid,'a monitor?',ws.getInstrument().getDetector(detid).isMonitor()

  # Get the Run object
  run = ws.run()
  print 'Workspace contains',len(run.keys()),'logs'
  print 'Workspace has property INT1:',run.hasProperty('INT1')

Output
^^^^^^

.. testoutput:: ExLoadDataNoLogs

  Workspace has 2890 spectra
  Is detector 1001 a monitor? True
  Workspace contains 29 logs
  Workspace has property INT1: False

Example 3: exclude monitors
###########################

.. testcode:: ExLoadDataNoMonitors

  # Load a RAW file without the monitors
  ws = Load('HRP39180.RAW',LoadMonitors='Exclude')
  print 'Workspace has',ws.getNumberHistograms(),'spectra'
  # Get a detector in histogram 1024 of the workspace
  detid = ws.getSpectrum(1024).getDetectorIDs()[0]
  print 'Is detector',detid,'a monitor?',ws.getInstrument().getDetector(detid).isMonitor()

  # Get the Run object
  run = ws.run()
  print 'Workspace contains',len(run.keys()),'logs'
  print 'Workspace has property INT1:',run.hasProperty('INT1')

Output
^^^^^^

.. testoutput:: ExLoadDataNoMonitors

  Workspace has 2888 spectra
  Is detector 901000 a monitor? False
  Workspace contains 35 logs
  Workspace has property INT1: True

Example 4: load monitors separately
###################################

.. testcode:: ExLoadDataSepMonitors

  # Load a RAW file, load the monitors into a separate workspace
  dataws,monitorws = Load('HRP39180.RAW',LoadMonitors='Separate')
  print 'Data workspace has',dataws.getNumberHistograms(),'spectra'

  print 'Monitor workspace has',monitorws.getNumberHistograms(),'spectra'
  # Check that the detector in the first histogram of the monitor workspace is a monitor
  detid = monitorws.getSpectrum(0).getDetectorIDs()[0]
  print 'Is detector',detid,'a monitor?',monitorws.getInstrument().getDetector(detid).isMonitor()

  # Check that the detector in the second histogram of the monitor workspace is a monitor
  detid = monitorws.getSpectrum(1).getDetectorIDs()[0]
  print 'Is detector',detid,'a monitor?',monitorws.getInstrument().getDetector(detid).isMonitor()

Output
^^^^^^

.. testoutput:: ExLoadDataSepMonitors

  Data workspace has 2888 spectra
  Monitor workspace has 2 spectra
  Is detector 1001 a monitor? True
  Is detector 1002 a monitor? True


.. categories::
