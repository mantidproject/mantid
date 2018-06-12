.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

LoadSINQ loads SINQ NeXus files. The algorithm calculates the file name
from the instrument, year and numor and tries to locate the file, both
at SINQ standard paths as well as the data directories configured for
Mantid. Then it calls :ref:`algm-LoadSINQFile` for the located data file.

The Mantid standard Load algorithm selects based on file extensions. The
file extensions used at SINQ, mainly .hdf and .h5, were already taken.
Thus the need for a separate loader.

Usage
-----

The following usage example loads a POLDI data file using the instrument name, year and numor.

.. testcode:: ExLoadSINQPOLDI

    poldi_data = LoadSINQ(Instrument = "POLDI", Year = 2013, Numor = 6904)
    
    print("Poldi sample 6904 has {} histograms.".format(poldi_data.getNumberHistograms()))

Output:
    
.. testoutput:: ExLoadSINQPOLDI

    Poldi sample 6904 has 400 histograms.


.. categories::

.. sourcelink::
