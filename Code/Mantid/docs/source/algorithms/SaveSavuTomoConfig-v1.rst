.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm writes a file with tomographic reconstruction
parameterization (configuration) file using the format of the savu
tomography reconstruction pipeline
(`<https://github.com/DiamondLightSource/Savu>`__). The parameters are
taken from a list of `TableWorkspace
<http://www.mantidproject.org/TableWorkspace>`_ workspaces. The data
in every workspace is expected to have four columns, with each row
specifying one plugin. For every plugin four character string
attributes (four columns) must be given, in this order: id, parameters
(JSON string of name-value pairs), name, and cite (citation
information about plugin documentation and authors).

This algorithm is used by the IMAT tomography reconstruction interface
(GUI) to save configuration files that can then be used to run
tomography reconstruction jobs using the savu pipeline.

Usage
-----

**Example**

.. testcode:: SaveSavuTomoConfig

   import os.path
   tws_name = 'saveSavuTomoTest'
   tws = CreateEmptyTableWorkspace(OutputWorkspace=tws_name)
   tws.addColumn('str', 'ID')
   tws.addColumn('str', 'Parameters')
   tws.addColumn('str', 'Name')
   tws.addColumn('str', 'Cite')
   tws.addRow(['savu.id1', '{"param11": val1', 'plugin name1', 'cite info1'])
   tws.addRow(['savu.id2', '{"param21": val2', 'plugin name2', 'cite info2'])
   print "Columns: %d" % tws.columnCount()
   print "Rows: %d" % tws.rowCount()
   out_fname = 'saveSavuTomoTest.nxs'
   SaveSavuTomoConfig(Filename=out_fname, InputWorkspaces='saveSavuTomoTest')
   res = os.path.isfile(out_fname)
   print "Save result: ", res

.. testcleanup:: SaveSavuTomoConfig

    DeleteWorkspace(tws)
    os.remove(out_fname)

Output:

.. testoutput:: SaveSavuTomoConfig

   Columns: 4
   Rows: 2
   Save result:  True

.. categories::
