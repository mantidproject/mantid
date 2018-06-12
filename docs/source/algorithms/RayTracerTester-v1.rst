.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Algorithm to test ray tracer by spraying evenly spaced rays around. Only
for debugging / testing.

Usage
-----

**Example - MARI**  

.. testcode:: Ex1

  import os
  inst_dir = config["instrumentDefinition.directory"]
  inst_file_path = os.path.join(inst_dir,"MARI_Definition.xml")
  
  ws=RayTracerTester(inst_file_path)


.. categories::

.. sourcelink::
