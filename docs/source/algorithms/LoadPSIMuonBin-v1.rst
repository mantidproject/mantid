.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

The algorithm LoadPSIMuonBin will read in a .bin file from the PSI 
facility, from one of the various machines that use that format.
The file name can be an absolute or relative path and should have the
extension .bin. The file should only be loaded if the first two bytes
are "1N" representing it as the format that this will load.

LoadPSIMuonBin is capable of loading in accompanying temperature files.
There are two options for achieving this. If no file is provided and 
the temperature file searching enabled (it is by default), then it 
will recursively search for the file in the current directory of the
data file and up to 3 directories deeper than the file.

Any temperature data loaded in from a separate file will be available 
from the resultant workspace's sample logs, as a number series that 
is plottable.

Errors
######

For errors the Poisson distribution is used.

Child Algorithms used
#####################

The ChildAlgorithms used by LoadPSIMuonBin are:

* :ref:`algm-AddSampleLog-v1` - Adds data to the Sample Log of the workspace

.. categories::

.. sourcelink::
