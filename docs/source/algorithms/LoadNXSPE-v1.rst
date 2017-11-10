.. algorithm::

.. summary::

.. alias::

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

Usage
-----

See :ref:`algm-SaveNXSPE` usage examples where Save and Load NXSPE operations are tested together.

.. categories::

.. sourcelink::
