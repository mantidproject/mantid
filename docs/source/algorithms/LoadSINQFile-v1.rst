.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

LoadSINQFile is a wrapper algorithm around :ref:`algm-LoadFlexiNexus`. It locates a
suitable dictionary file for the instrument in question and then goes
away to call LoadFlexiNexus with the right arguments. It also performs
any other magic which might be required to get the data in the right
shape for further processing in Mantid.

In most cases, the algorithm :ref:`algm-LoadSINQ` can be used instead, especially if the file is located in one of the standard data directories. LoadSINQFile is mainly useful if a SINQ-file from a specific path or a non-standard file name has to be loaded.

Usage
-----

.. testcode:: ExLoadSINQFilePOLDI

    poldi_data = LoadSINQFile(Filename = "poldi2013n006904.hdf", Instrument = "POLDI")

    print("Poldi sample 6904 has {} histograms.".format(poldi_data.getNumberHistograms()))

Output:

.. testoutput:: ExLoadSINQFilePOLDI

    Poldi sample 6904 has 400 histograms.

.. categories::

.. sourcelink::
