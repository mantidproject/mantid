.. _FindRootNodes:

===============
Find Root Nodes
===============

The :code:`FindRootNodes` component filters an unordered set of row locations down to a
lexicographically ordered set of row locations where each location corresponds to the root
of a subtree.


.. image::  ../../images/find_subtree_roots.svg
   :align: center
   :width: 800px

Where multiple solutions are possible this algorithm will find the set of subtrees with
the minimal number of subtrees by grouping connected row locations together into a
single subtree.

The algorithm used to perform this conversion makes a simplifying assumption that the roots of all
subtrees in the result share a common parent. If this assumption is untrue then the input set
is unsuitable and the algorithm will return an empty optional.

.. image::  ../../images/subtree_fail.svg
   :align: center
   :width: 800px


This algorithm is used by :doc:`../API/JobTreeView` in the :code:`selectedSubtreeRoots` method which is
required to implement non-trivial copy and paste.
