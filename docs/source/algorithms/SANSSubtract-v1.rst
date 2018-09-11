.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Subtract background from an I(Q) distribution.

The DataDistribution and Background properties can either be the name of
a workspace or a file path. If a file path is provided, it will be
loaded and assumed to be in units of Q.

The output workspace will be equal to:

| `` Output = DataDistribution - ScaleFactor * Background + Constant``
| `` ``

The Dq values are propagated from the DataDistribution workspace to the
output workspace as-is.

If the OutputDirectory property is filled, the output workspace will be
written to disk. Two files will be produced, a 4 column ASCII file and a
CanSAS XML file.

.. categories::

.. sourcelink::
