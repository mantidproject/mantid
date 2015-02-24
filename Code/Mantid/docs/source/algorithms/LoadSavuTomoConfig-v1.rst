.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm reads a tomographic reconstruction parameterization
(configuration) file and stores the configuration in a `TableWorkspace
<http://www.mantidproject.org/TableWorkspace>`_. The file is expected
to follow the format used in the savu tomography reconstruction
pipeline `<https://github.com/DiamondLightSource/Savu>`__. These files
specify a sequence of plugins to be used for tomographic
reconstruction. For each plugin four fields are given in this order:
id, parameters, name, and cite. All fields are character strings. The
parameters field is formatted as a JSON string of name,value
pairs. The workspace produced has one row for every plugin found in
the input file, and four columns of string type.

This algorithm is used by the IMAT tomography reconstruction interface
(GUI) to load and display configurations that can then be edited and
saved.

Usage
-----

**Example**

.. testcode:: LoadSavuTomoConfig

    tws = LoadSavuTomoConfig("savu_test_data_process03.nxs", OutputWorkspace='savu_tomo_config')
    print "Number of columns: %d" % tws.columnCount()
    print "Number of rows / processing plugins: %d" % tws.rowCount()
    print "Cell 0,0: %s" % tws.cell(0,0)
    print "Cell 0,1: %s" % tws.cell(0,1)
    print "Cell 0,2: %s" % tws.cell(0,2)
    print "Cell 0,3: %s" % tws.cell(0,3)
    print "Cell 1,0: %s" % tws.cell(1,0)
    print "Cell 1,1: %s" % tws.cell(1,1)
    print "Cell 1,2: %s" % tws.cell(1,2)
    print "Cell 1,3: %s" % tws.cell(1,3)
    print "Cell 2,0: %s" % tws.cell(2,0)
    print "Cell 2,1: %s" % tws.cell(2,1)
    print "Cell 2,2: %s" % tws.cell(2,2)
    print "Cell 2,3: %s" % tws.cell(2,3)

.. testcleanup:: LoadSavuTomoCopnfig

    DeleteWorkspace(tws)

Output:

.. testoutput:: LoadSavuTomoConfig

    Number of columns: 4
    Number of rows / processing plugins: 3
    Cell 0,0: savu.plugins.timeseries_field_corrections
    Cell 0,1: {}
    Cell 0,2: Timeseries Field Corrections
    Cell 0,3: Not available
    Cell 1,0: savu.plugins.median_filter
    Cell 1,1: {"kernel_size": [1, 3, 3]}
    Cell 1,2: Median Filter
    Cell 1,3: Not available
    Cell 2,0: savu.plugins.simple_recon
    Cell 2,1: {"center_of_rotation": 86}
    Cell 2,2: Simple Reconstruction
    Cell 2,3: Not available

.. categories::
