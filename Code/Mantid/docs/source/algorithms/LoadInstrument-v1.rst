.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Loads an instrument definition file (`IDF <http://www.mantidproject.org/InstrumentDefinitionFile>`_)
into a workspace, which contains information about detector positions,
their geometric shape, slit properties, links between values stored in
ISIS log-files and components of the instrument and so on. For more on
IDFs see: `InstrumentDefinitionFile <http://www.mantidproject.org/InstrumentDefinitionFile>`_.

By default the algorithm will write a 1:1 map between the spectrum
number and detector ID. Any custom loading algorithm that calls this as
a Child Algorithm will therefore get this 1:1 map be default. If the
custom loader is to write its own map then it is advised to set
``RewriteSpectraMap`` to false to avoid extra work.

The instrument to load can be specified by either the InstrumentXML,
Filename and InstrumentName properties (given here in order of
precedence if more than one is set). At present, if the InstrumentXML is
used the InstrumentName property should also be set.

.. categories::
