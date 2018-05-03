.. _ExtractSubtrees:

================
Extract Subtrees
================

The :code:`ExtractSubtrees` component converts a list of row cell contents and a list of row
locations into a structured list of Subtrees this is better described by the illustration below.


The algorithm used to perform this conversion makes a simplifying assumption that the roots of all
subtrees in the result all share a common parent. If this assumption is untrue then the algorithm
will return an empty optional.



This algorithm is used by the :code:`selectedSubtrees` method on the :code:`JobTreeView` which is
required to implement non-trivial copy and paste.
