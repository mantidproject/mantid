.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Save an `MDEventWorkspace <http://www.mantidproject.org/MDEventWorkspace>`_ or a
:ref:`MDHistoWorkspace <MDHistoWorkspace>` to a .nxs file. The
workspace's current box structure and entire list of events is
preserved. The resulting file can be loaded via :ref:`LoadMD <algm-LoadMD>`.

If you specify MakeFileBacked, then this will turn an in-memory
workspace to a file-backed one. Memory will be released as it is written
to disk.

If you specify UpdateFileBackEnd, then any changes (e.g. events added
using the PlusMD algorithm) will be saved to the file back-end.

Usage
-----

.. testcode:: DoIt

    ws = CreateMDHistoWorkspace(SignalInput='1,2,3,4,5,6,7,8,9', ErrorInput='1,1,1,1,1,1,1,1,1', Dimensionality='2',
                                Extents='-1,1,-1,1', NumberOfBins='3,3', Names='A,B', Units='U,T')
    import os
    savefile = os.path.join(config["default.savedirectory"], "mdhws.nxs")
    SaveMD(ws, Filename=savefile, Version=1)
    print "File created:", os.path.exists(savefile)

Output:

.. testoutput:: DoIt

    File created: True

.. testcleanup:: DoIt

    import os
    os.remove(savefile)

.. categories::
