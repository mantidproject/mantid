.. algorithm::

.. summary::

.. relatedalgorithms::

:ref:`algm-LoadEmptyInstrument`

.. properties::

Description
-----------
This algorithm performs a similar function to :ref:`algm-LoadEmptyInstrument`, but builds the instrument geometry from a `NeXus format <https://www.nexusformat.org/>`_ (hdf5) file instead of an Instrument Definition File. 

A fake workspace is created, 1 spectra for every detector in the instrument file. The workspace is comprised of histograms with two bin edges 0 and 1. The data is meanlingless. The workspace merely provides a host for attaching the loaded instrument to. 

Usage
-----

**Example - Summing all the counts in a workspace**

.. testcode:: exLoadEmptyNexusGeometry

    ws = LoadEmptyNexusGeometry(Filename="LOKI_Definition.hdf5")
    print("The workspace contains {} spectra".format(ws.getNumberHistograms()))
    print("Instrument is {}".format(ws.getInstrument().getName()))

Output:

.. testoutput:: exLoadEmptyNexusGeometry

    The workspace contains 8000 spectra
    Instrument is LOKI


.. categories::

.. sourcelink::
