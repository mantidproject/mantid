.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm loads processed HDF5 files produced by LAMP software at the ILL. Only 1D and 2D data are supported. The output is a point data workspace.
The input file must have a tree structure similar to:

.. code-block:: python

  entry1
    |___data1
          |__DATA
          |__PARAMETERS
          |__X
          |__Y

Usage
-----

**Example - LoadLamp**

.. testcode:: ExLoadLamp

    ws = LoadLamp('ILL/LAMP/967067_LAMP.hdf')
    print("ws has {0} spectrum and {1} points".format(ws.getNumberHistograms(), ws.blocksize()))

Output:

.. testoutput:: ExLoadLamp

    ws has 1 spectrum and 3200 points

.. testcleanup:: ExLoadLamp

   mtd.clear()

.. categories::

.. sourcelink::
