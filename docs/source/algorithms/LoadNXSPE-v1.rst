.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Algorithm to load an NXSPE file into a workspace2D. It will create a new
instrument, that can be overwritten later by the LoadInstrument
algorithm.

**NOTE:** The final energy of indirect geometry data is not saved to this file.
For indirect data, we recommend to save the data in NXS format instead.

**NOTE:** In the current implementation, the rendering of the NXSPE
instrument is VERY memory intensive.

**NOTE:** This algorithm also sets to true the `distribution` flag on the output 
workspace. This is because all examples of workspaces saved to `NXSPE` format
by the reduction algorithms are distributions (signal is count rate and should
be multiplied by bin widths to get counts). :ref:`algm-SaveNXSPE` does not
require its input is a distribution, however, and the `NXSPE` format does not
have a distribution flag.

Usage
-----

See :ref:`algm-SaveNXSPE` usage examples where Save and Load NXSPE operations are tested together.

.. categories::

.. sourcelink::
