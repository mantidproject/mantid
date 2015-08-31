.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm is to extract plottable 1-D dataset(s) with
metadata from a MatrixWorkspace, serialize it into json format, and
dump the result to a file.

It currently only support MatrixWorkspace


Usage
-----

**Example:**

.. testcode:: ExSave1DPlottableJson

  import os
  
  dataws = Load(Filename="withplottables.nxs")
  out_json = "myplot.json"
  
  Save1DPlottableAsJson(InputWorkspace=dataws, JsonFilename=out_json, PlotName="myplot")


.. categories::

.. sourcelink::
