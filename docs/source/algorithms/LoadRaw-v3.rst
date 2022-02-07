.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

The LoadRaw algorithm stores data from the :ref:`RAW file <RAW File>` in a
:ref:`Workspace2D <Workspace2D>`, which will naturally contain histogram
data with each spectrum going into a separate histogram. The time bin
boundaries (X values) will be common to all histograms and will have
their :ref:`units <Unit Factory>` set to time-of-flight. The Y values will contain
the counts and will be unit-less (i.e. no division by bin width or
normalisation of any kind). The errors, currently assumed Gaussian, will
be set to be the square root of the number of counts in the bin.

Optional properties
###################

If only a portion of the data in the :ref:`RAW file <RAW File>` is
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
:ref:`Instrument <Instrument>`, SpectraToDetectorMap and
:py:obj:`sample objects <mantid.api.Sample>`. If the optional 'spectrum'
properties are set for a multiperiod dataset, then they will ignored.

If PeriodList property isn't empty then only periods listed there will be
loaded.

Subalgorithms used
##################

LoadRaw runs the following algorithms as child algorithms to populate
aspects of the output :ref:`Workspace <Workspace>`:

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
   :ref:`Instrument <Instrument>`.
-  :ref:`algm-LoadLog` - Will look for any log files in the same
   directory as the RAW file and load their data into the workspace's
   :py:obj:`sample objects <mantid.api.Sample>`.

Alternate Data Stream
#####################

RAW data files from the ISIS archive contain a second stream of data associated with
the same file object using the
`NTFS Alternate Data Stream <https://en.wikipedia.org/wiki/Fork_(file_system)#Microsoft>`_
feature. The alternate stream can be accessed by appending ``:checksum`` to the filename
when opening the file. The stream stores MD5 checksums of all of the files associated with
the RAW file including log files and the corresponding ISIS NeXus file that can be loaded
with :ref:`LoadISISNexus <algm-LoadISISNexus-v2>`.
The following shows an example of the content found in the ``:checksum`` stream::

    ad0bc56c4c556fa368565000f01e77f7 *IRIS00055132.log
    d5ace6dc7ac6c4365d48ee1f2906c6f4 *IRIS00055132.nxs
    9c70ad392023515f775af3d3984882f3 *IRIS00055132.raw
    66f74b6c0cc3eb497b92d4956ed8d6b5 *IRIS00055132_ICPdebug.txt
    e200aa65186b61e487175d5263b315aa *IRIS00055132_ICPevent.txt
    91be40aa4f54d050a9eb4abea394720e *IRIS00055132_ICPstatus.txt
    50aa2872110a9b862b01c6c83f8ce9a8 *IRIS00055132_Status.txt

The MD5 checksum of the RAW file itself is not affected by the presence of the alternate stream.
If the RAW file is copied to a filesystem that does not support the alternate stream then
the stream is dropped.

Previous Versions
-----------------

LoadRaw version 1 and 2 are no longer available in Mantid. Version 3 has
been validated and in active use for several years, if you really need a
previous version of this algorithm you will need to use an earlier
version of Mantid.

Usage
-----

.. include:: ../usagedata-note.txt

**Example 1: using defaults**

.. testcode:: ExLoadDataDefaults

  # Load a RAW file using all defaults
  ws = Load('HRP39180.RAW')
  print('Workspace has {} spectra'.format(ws.getNumberHistograms()))
  # Get a detector in histogram 1024 of the workspace
  detid = ws.getSpectrum(1024).getDetectorIDs()[0]
  print('Is detector {} a monitor? {}'.format(detid, ws.getInstrument().getDetector(detid).isMonitor()))

  # Get the Run object
  run = ws.run()
  print('Workspace contains {} logs'.format(len(run.keys())))
  print('Workspace has property INT1: {}'.format(run.hasProperty('INT1')))

Output:

.. testoutput:: ExLoadDataDefaults

  Workspace has 2890 spectra
  Is detector 1001 a monitor? True
  Workspace contains 35 logs
  Workspace has property INT1: True


**Example 2: load without the logs**

.. testcode:: ExLoadDataNoLogs

  # Load a RAW file without the logs
  ws = Load('HRP39180.RAW',LoadLogFiles=False)
  print('Workspace has {} spectra'.format(ws.getNumberHistograms()))
  # Get a detector in histogram 1024 of the workspace
  detid = ws.getSpectrum(1024).getDetectorIDs()[0]
  print('Is detector {} a monitor? {}'.format(detid, ws.getInstrument().getDetector(detid).isMonitor()))

  # Get the Run object
  run = ws.run()
  print('Workspace contains {} logs'.format(len(run.keys())))
  print('Workspace has property INT1: {}'.format(run.hasProperty('INT1')))

Output:

.. testoutput:: ExLoadDataNoLogs

  Workspace has 2890 spectra
  Is detector 1001 a monitor? True
  Workspace contains 29 logs
  Workspace has property INT1: False

**Example 3: exclude monitors**

.. testcode:: ExLoadDataNoMonitors

  # Load a RAW file without the monitors
  ws = Load('HRP39180.RAW',LoadMonitors='Exclude')
  print('Workspace has {} spectra'.format(ws.getNumberHistograms()))
  # Get a detector in histogram 1024 of the workspace
  detid = ws.getSpectrum(1024).getDetectorIDs()[0]
  print('Is detector {} a monitor? {}'.format(detid, ws.getInstrument().getDetector(detid).isMonitor()))

  # Get the Run object
  run = ws.run()
  print('Workspace contains {} logs'.format(len(run.keys())))
  print('Workspace has property INT1: {}'.format(run.hasProperty('INT1')))

Output:

.. testoutput:: ExLoadDataNoMonitors

  Workspace has 2888 spectra
  Is detector 901000 a monitor? False
  Workspace contains 35 logs
  Workspace has property INT1: True

**Example 4: load monitors separately**

.. testcode:: ExLoadDataSepMonitors

  # Load a RAW file, load the monitors into a separate workspace
  dataws,monitorws = Load('HRP39180.RAW',LoadMonitors='Separate')
  print('Data workspace has {} spectra'.format(dataws.getNumberHistograms()))

  print('Monitor workspace has {} spectra'.format(monitorws.getNumberHistograms()))
  # Check that the detector in the first histogram of the monitor workspace is a monitor
  detid = monitorws.getSpectrum(0).getDetectorIDs()[0]
  print('Is detector {} a monitor? {}'.format(detid, monitorws.getInstrument().getDetector(detid).isMonitor()))

  # Check that the detector in the second histogram of the monitor workspace is a monitor
  detid = monitorws.getSpectrum(1).getDetectorIDs()[0]
  print('Is detector {} a monitor? {}'.format(detid, monitorws.getInstrument().getDetector(detid).isMonitor()))

Output:

.. testoutput:: ExLoadDataSepMonitors

  Data workspace has 2888 spectra
  Monitor workspace has 2 spectra
  Is detector 1001 a monitor? True
  Is detector 1002 a monitor? True


.. categories::

.. sourcelink::
