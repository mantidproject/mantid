.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm writes a file with tomographic reconstruction
parameters (configuration). The parameters are taken from a list of
`TableWorkspace <http://www.mantidproject.org/TableWorkspace>`_. The
data in every workspace is expected to have four columns, with each
row specifying one plugin. For every plugin four character string
attributes (four columns) must be given, in this order: id, parameters
(JSON string of name-value pairs), name, and cite (citation
information about plugin documentation and authors).

.. comment
   Usage
   -----
   **Example**

   .. testcode:: SaveTomoConfig

      # TODO - polish this
      import mantid, os.path
      tws = CreateEmptyTableWorkspace(OutputWorkspace="saveTomoTest")
      tws.addColumn('str', 'id')
      tws.addColumn('str', 'params')
      tws.addColumn('str', 'name')
      tws.addColumn('str', 'cite')
      tws.addRow(['id1', 'params1', 'name1', 'cite1'])
      tws.addRow(['id2', 'params2', 'name2', 'cite2'])
      print "Columns: ", tws.columnCount()
      print "Rows: ", tws.rowCount()
      out_fname = 'saveTomoTest.nxs'
      SaveTomoConfig(Filename=out_fname, InputWorkspaces='saveTomoTest')
      res = os.path.isfile(fname)
      print "Save result: ", res
      # TODO
      # should be more properly tested when LoadTomoConfig is in

   Output:

   .. testoutput:: SaveTomoConfig

      Columns:  4
      Rows:  1
      Save result:  True

.. categories::
