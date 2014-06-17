.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

SaveILLCosmosAscii is an export-only Ascii-based save format with no
associated loader. It is based on a python script by Maximilian Skoda,
written for the ISIS Reflectometry GUI

Limitations
###########

While Files saved with SaveILLCosmosAscii can be loaded back into mantid
using LoadAscii, the resulting workspaces won't be usful as the data
written by SaveILLCosmosAscii is not in the normal X,Y,E,DX format.

.. categories::
