.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Description
-----------

LoadSINQ loads SINQ NeXus files. The algorithm calculates the file name
from the instrument, year and numor and tries to locate the file. Both
at SINQ standard paths as well as the data directories configured for
Mantid. Then it calls LoadSINQFile for the located data file.

The Mantid standard Load algorithm selects based on file extensions. The
file extensions used at SINQ, mainly .hdf and .h5, were already taken.
Thus the need for a separate loader.

.. categories::
