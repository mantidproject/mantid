.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm is to enable you to inspect the instrument geometry without having
to load a full data file. 

This algorithm works with both :ref:`Instrument Definition Files <InstrumentDefinitionFile>` and `NeXus format <https://www.nexusformat.org/>`_ (hdf5) files.

Input files only provide the instrument geometry. The instrument
is referred to as being empty because there is no real data associated with
it. The algorithm provides some limited options to determine what fake data will be used.


Usage
-----

**Example - Loading an instrument IDF from an XML file**

.. testcode:: exLoadEmptyInstrumentXML

    import os

    wsOut = LoadEmptyInstrument(Filename="INES_Definition.xml")
    print("The workspace contains {} spectra".format(wsOut.getNumberHistograms()))

Output:

.. testoutput:: exLoadEmptyInstrumentXML

    The workspace contains 146 spectra

**Example - Loading an instrument from an NeXus file**

.. testcode:: exLoadEmptyInstrumentNXS

    ws = LoadEmptyInstrument(Filename="LOKI_Definition.hdf5")
    print("The workspace contains {} spectra".format(ws.getNumberHistograms()))
    print("Instrument is {}".format(ws.getInstrument().getName()))

Output:

.. testoutput:: exLoadEmptyInstrumentNXS

    The workspace contains 8000 spectra
    Instrument is LOKI
.. categories::

.. sourcelink::
