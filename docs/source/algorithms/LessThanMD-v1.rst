.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Perform the < (less-than) boolean operation on two MDHistoWorkspaces or
a MDHistoWorkspace and a scalar. The output workspace has a signal of
0.0 to mean "false" and a signal of 1.0 to mean "true". Errors are 0.

For two MDHistoWorkspaces, the operation is performed
element-by-element.

For a MDHistoWorkspace and a scalar, the operation is performed on each
element of the output.

.. categories::

.. sourcelink::
