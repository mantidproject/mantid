.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Save a `MDEventWorkspace <MDEventWorkspace>`__ to a .nxs file. The
workspace's current box structure and entire list of events is
preserved. The resulting file can be loaded via :ref:`algm-LoadMD`.

If you specify MakeFileBacked, then this will turn an in-memory
workspace to a file-backed one. Memory will be released as it is written
to disk.

If you specify UpdateFileBackEnd, then any changes (e.g. events added
using the PlusMD algorithm) will be saved to the file back-end.

.. categories::
