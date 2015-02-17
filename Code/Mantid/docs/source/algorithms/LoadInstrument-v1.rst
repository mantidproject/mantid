.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Loads an instrument definition file (:ref:`IDF <InstrumentDefinitionFile>`)
into a workspace, which contains information about detector positions,
their geometric shape, slit properties, links between values stored in
ISIS log-files and components of the instrument and so on. For more on
IDFs see: :ref:`InstrumentDefinitionFile <InstrumentDefinitionFile>`.

By default the algorithm will write a 1:1 map between the spectrum
number and detector ID. Any custom loading algorithm that calls this as
a Child Algorithm will therefore get this 1:1 map be default. If the
custom loader is to write its own map then it is advised to set
``RewriteSpectraMap`` to false to avoid extra work.

The instrument to load can be specified by either the InstrumentXML,
Filename and InstrumentName properties (given here in order of
precedence if more than one is set). At present, if the InstrumentXML is
used the InstrumentName property should also be set.

Usage
-----

**Example - Load instrument to a workspace:**

.. testcode:: exLoadInstrument

   # create sample workspace
   ws=CreateSampleWorkspace();
   inst0=ws.getInstrument();

   print "Default workspace has instrument: {0} with {1} parameters".format(inst0.getName(),len(inst0.getParameterNames()));

   # load MARI
   det=LoadInstrument(ws,InstrumentName='MARI')
   inst1=ws.getInstrument();

   print "Modified workspace has instrument: {0} with {1} parameters".format(inst1.getName(),len(inst1.getParameterNames()));
   print "Instrument {0} has the following detectors: ".format(inst1.getName()),det


**Output:**

.. testoutput:: exLoadInstrument

   Default workspace has instrument: basic_rect with 0 parameters
   Modified workspace has instrument: MARI with 72 parameters
   Instrument MARI has the following detectors:  [1 2 3]



.. categories::
