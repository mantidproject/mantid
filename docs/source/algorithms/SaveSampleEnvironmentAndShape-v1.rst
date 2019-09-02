.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Saves out the sample and environment on a workspace as a binary .stl file, the sample shape and environment must have been set using :ref:`LoadSampleShape <algm-LoadSampleShape>` and :ref:`LoadSampleEnvironment <algm-LoadSampleEnvironment>`, as this algorithm only supports shapes stored as a mesh. 

The output is of the following type:

* ``*.stl`` stereolithography `https://en.wikipedia.org/wiki/STL_(file_format) <https://en.wikipedia.org/wiki/STL_(file_format)>`_
  This is a file format consisting of a list of faces specified by their vertex coordinates.
  The file will be in Binary format, the Header will contain information about when the file was created, and the normals and attribute code are unset. 
  The vertices are in the standard order (counter clockwise when viewed from the outside).


.. categories::

.. sourcelink::
