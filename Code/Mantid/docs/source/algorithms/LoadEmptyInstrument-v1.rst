.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm is to enable you to look at an instrument without having
to load a full data file. Instead of loading a data file, it loads the
:ref:`InstrumentDefinitionFile <InstrumentDefinitionFile>` for the
instrument, which has information about the instrument. The instrument
is referred to as being empty because there is no data associated with
it.


Usage
-----

**Example - Summing all the counts in a workspace**

.. testcode:: exLoadEmptyInstrument

    import os

    inesPath = os.path.join(ConfigService.getInstrumentDirectory(),"INES_Definition.xml")

    wsOut = LoadEmptyInstrument(inesPath)

    print "The workspace contains %i spectra" % wsOut.getNumberHistograms()

Output:

.. testoutput:: exLoadEmptyInstrument

    The workspace contains 146 spectra


.. categories::
