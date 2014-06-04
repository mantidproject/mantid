.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Takes two arrays of signal and error values, as well as information
describing the dimensionality and extents, and creates a
MDHistoWorkspace (histogrammed multi-dimensional workspace). The
*SignalInput* and *ErrorInput* arrays must be of equal length and have a
length that is equal to the product of all the comma separated arguments
provided to **NumberOfBins**. The order in which the arguments are
specified to each of the properties (for those taking multiple
arguments) is important, as they are assumed to match by the order they
are declared. For example, specifying **Names**\ ='A,B' and
**Units**\ ='U1,U2' will generate two dimensions, the first with a name
of *A* and units of *U1* and the second with a name of *B* and units of
*U2*. The same logic applies to the **Extents** inputs. Signal and Error
inputs are read in such that, the first entries in the file will be
entered across the first dimension specified, and the zeroth index in
the other dimensions. The second set of entries will be entered across
the first dimension and the 1st index in the second dimension, and the
zeroth index in the others.

Alternatives
------------

A very similar algorithm to this is
:ref:`algm-ImportMDHistoWorkspace`, which takes it's
input signal and error values from a text file rather than from arrays.
Another alternative is to use :ref:`algm-ConvertToMD` which works
on MatrixWorkspaces, and allows log values to be included in the
dimensionality.

.. categories::
