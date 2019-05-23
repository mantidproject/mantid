.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Loads a shape into the sample of a workspace. The Sample will be rotated according to the rotation of the goniometer on the workspace, if there is one. The first angle used in `SetGoniometer` will be applied last.

The following types of input file are supported:

* ``*.stl`` stereolithography `https://en.wikipedia.org/wiki/STL_(file_format) <https://en.wikipedia.org/wiki/STL_(file_format)>`_
  This is a file format consisting of a list of faces specified by their vertex coordinates.
  The file may be in ASCII or Binary format, and all the faces must be triangular. 
  The normals are ignored, but the vertices must be in the order required by the standard 
  (counter-clockwise when viewed from outside).
* ``*.off`` Object File Format `https://en.wikipedia.org/wiki/OFF_(file_format) <https://en.wikipedia.org/wiki/OFF_(file_format)>`_
  This is a file format consisting of a list of vertices and a list of faces specified by the position of 
  each vertex in the list of vertices.
  The file must is in ASCII for and all the faces must be triangular. There are no normals.
  The vertices of a face must be ordered counter-clockwise when viewed from outside.

.. categories::

.. sourcelink::
