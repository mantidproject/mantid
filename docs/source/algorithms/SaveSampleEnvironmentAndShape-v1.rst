.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Saves out the sample and environment on a workspace as a binary .stl file, the sample shape and environment must have been set using :ref:`LoadSampleShape <algm-LoadSampleShape>` and :ref:`LoadSampleEnvironment <algm-LoadSampleEnvironment>`, as this algorithm only supports shapes stored as a mesh.

The sample and environment are saved as defined, with the goniometer at zero, even if the sample was turned by the goniometer as it was loaded. Loading the file again therefore turns them by the goniometer once. The file cannot record which parts of the environment are :ref:`on the goniometer <SampleEnvironment_goniometer>`, so set ``OnGoniometer`` in :ref:`LoadSampleEnvironment <algm-LoadSampleEnvironment>` when reloading an environment whose other parts turn with the sample.

The output is of the following type:

* ``*.stl`` stereolithography `https://en.wikipedia.org/wiki/STL_(file_format) <https://en.wikipedia.org/wiki/STL_(file_format)>`_
  This is a file format consisting of a list of faces specified by their vertex coordinates.
  The file will be in Binary format, the Header will contain information about when the file was created, and the normals and attribute code are unset.
  The vertices are in the standard order (counter clockwise when viewed from the outside).
* ``.3mf`` https://3mf.io This is a 3D printing format that allows multiple objects to be stored in a single file, each with a representation similar to the .stl format ie a list of faces and vertices


.. categories::

.. sourcelink::
