.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Loads the instrument geometry from either an instrument definition file
(:ref:`IDF <InstrumentDefinitionFile>`) or a Nexus file into a workspace. The
geometry contains information about detector positions, their geometric shape,
slit properties, links between values stored in log-files and components of the
instrument and so on. For more on IDFs see:
:ref:`InstrumentDefinitionFile <InstrumentDefinitionFile>`.

By default the algorithm will write a 1:1 map between the spectrum
number and detector ID. Any custom loading algorithm that calls this as
a Child Algorithm will therefore get this 1:1 map be default. If the
custom loader is to write its own map then it is advised to set
``RewriteSpectraMap`` to false to avoid extra work.

The instrument to load can be specified by either the ``InstrumentXML`` (IDFs
only), ``Filename`` and ``InstrumentName`` properties (given here in order of
precedence if more than one is set). At present, if the ``InstrumentXML`` is
used the ``InstrumentName`` property should also be set.

The ``Filename`` can either be an absolute or relative path. In the latter case,
both the ``datasearch.directories`` and instrument directories (accessible via
``getInstrumentDirectories()`` and ``setInstrumentDirectories()``) are searched for
the specified file. The filename is required to be of the form
``InstrumentName + _Definition + Identifier + extension``.
The identifier then is the part of a filename that identifies the instrument
definition valid at a given date. If several instrument files files are valid at
the given date the file with the most recent from date is selected. If no such
files are found the file with the latest from date is selected.

If only the ``InstrumentName`` is specified, a filename is built on-the-fly to
have the form ``InstrumentName + _Definition.(xml|hdf5|nxs)``. Short instrument
names can also be used, such as ``seq``; they get translated to long instrument
names (e.g. ``SEQUOIA``) through the ``ConfigServiceImp::getInstrument().name()``
method.

Usage
-----

**Example - Load instrument to a workspace:**

.. testcode:: exLoadInstrument

   # create sample workspace
   ws1 = CreateSampleWorkspace();
   inst1 = ws1.getInstrument();
   print("Default workspace has instrument: {0} with {1} parameters".format(inst1.getName(),len(inst1.getParameterNames())))

   # load MARI from instrument name
   print("===========================")
   mon1 = LoadInstrument(ws1, InstrumentName="MARI", RewriteSpectraMap=True)
   inst1 = ws1.getInstrument()
   di1 = ws1.detectorInfo()
   ci1 = ws1.componentInfo()
   print("Modified workspace {0} has instrument: {1}".format(ws1.getName(), inst1.getName()))
   print("Instrument {0} has {1} components, including {2} monitors and {3} detectors".format(inst1.getName(), ci1.size(), len(mon1), di1.size()))

   # load LOKI from file name
   print("===========================")
   ws2 = CreateSampleWorkspace();
   mon2 = LoadInstrument(ws2, FileName="LOKI_Definition.hdf5", RewriteSpectraMap=True)
   inst2 = ws2.getInstrument()
   di2 = ws2.detectorInfo()
   ci2 = ws2.componentInfo()
   print("Workspace {0} has instrument: {1}".format(ws2.getName(), inst2.getName()))
   print("Instrument {0} has {1} components, including {2} monitors and {3} detectors".format(inst2.getName(), ci2.size(), len(mon2), di2.size()))

**Output:**

.. testoutput:: exLoadInstrument

   Default workspace has instrument: basic_rect with 0 parameters
   ===========================
   Modified workspace ws1 has instrument: MARI
   Instrument MARI has 963 components, including 3 monitors and 921 detectors
   ===========================
   Workspace ws2 has instrument: LOKI
   Instrument LOKI has 8011 components, including 0 monitors and 8000 detectors



.. categories::

.. sourcelink::
