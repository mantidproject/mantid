.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Loads an environment into the sample of a workspace, either replacing the current environment, or adding to it. The newly added Environment can be translated by a vector, and rotated by an angle along each axis applied in order X,Y,Z. You may also set a material for the environment to be loaded, this follows the same inputs as :ref:`algm-SetSampleMaterial`.

The following types of input file are supported:

* ``*.stl`` stereolithography `https://en.wikipedia.org/wiki/STL_(file_format) <https://en.wikipedia.org/wiki/STL_(file_format)>`_
  This is a file format consisting of a list of faces specified by their vertex coordinates.
  The file may be in ASCII or Binary format, and all the faces must be triangular. 
  The normals are ignored, but the vertices must be in the order required by the standard 
  (counter-clockwise when viewed from outside).


.. categories::

.. sourcelink::
