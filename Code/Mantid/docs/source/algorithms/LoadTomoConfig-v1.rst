.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm reads a tomographic reconstruction parameterization
(configuration) file and stores the configuration in a `TableWorkspace
<http://www.mantidproject.org/TableWorkspace>`_. The file is expected
to follow the following format. It contains a sequence of plugins to
be used for tomographic reconstruction. For each plugin four fields
are specificed in this order: id, parameters, name, and cite. All
fields are character strings. The parameters field is formatted as a
JSON string of name,value pairs. The workspace produced has one row
for every plugin found in the input file, and four columns of string
type.

Usage
-----

**Example**

.. testcode:: LoadTomoConfig

    # TODO: check, put the example file, and finalize
    tws = LoadNexusMonitors("CNCS_7860_event.nxs")
    print "Number of columns: ", tws.columnCount()
    print "Number of rows / processing plugins: ", tws.rowCount()
    print "Cell 0,0: ", tws.cell(0,0)
    print "Cell 0,1: ", tws.cell(0,1)
    print "Cell 0,2: ", tws.cell(0,2)
    print "Cell 0,3: ", tws.cell(0,3)
    print "Cell 2,0: ", tws.cell(2,0)
    print "Cell 2,1: ", tws.cell(2,1)
    print "Cell 2,2: ", tws.cell(2,2)
    print "Cell 2,3: ", tws.cell(2,3)

Output:

.. testoutput:: LoadTomoConfig

    # TODO: check and finalize
    Number of columns: 4
    Number of rows / processing plugins: 3
    Cell 0,0: id-string
    Cell 0,1: parameters-string
    Cell 0,2: pluging name
    Cell 0,3: cite
    Cell 1,0: id-string
    Cell 1,1: parameters-string
    Cell 1,2: pluging name
    Cell 1,3: cite

.. categories::
