
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm masks the bins in ``InputWorkspace`` which lie in :math:`X` range that is not covered by ``ComparisonWorkspace``. The ``MaskPartiallyOverlapping`` flag affect the behavior with regards to bins which are partially covered by ``ComparisonWorkspace``. The algorithm works only with the X data sorted in ascending order.

The algorithm currently applies the default masking weight to the bins which does not clear the data.

Optimizations
#############

Some small optimizations are possible via ``CheckSortedX`` and ``RaggedInputs``. Make sure the input workspaces fill the expectations before using these properties!

- If there is no doubt that X data in ``InputWorkspace`` and ``ComparisonWorkspace`` is sorted, the checking for ascending X can be skipped by setting ``CheckSortedX`` to ``False``.
- If ``RaggedInputs`` is set to ``'Check'`` (the default), the algorithm will check if both ``InputWorkspace`` and ``ComparisonWorkspace`` are :ref:`ragged workspaces <Ragged_Workspace>` and choose the processing method accordingly. The test can be skipped by setting ``RaggedInputs`` to ``'Ragged'`` or ``'Common Bins'`` which forces a specific processing method.

Usage
-----

**Example - MaskNonOverlappingBins**

.. testcode:: MaskNonOverlappingBinsExample

   bigWS = CreateSampleWorkspace(XMin=0, XMax=20000)
   smallWS = CreateSampleWorkspace(XMin=9000, XMax=11000)
   masked = MaskNonOverlappingBins(bigWS, smallWS)
   print('It is not (yet) possible to access the bin masking information in Python.')
   print('Please check that the correct bins are grayed out in the data view.')

Output:

.. testoutput:: MaskNonOverlappingBinsExample

   It is not (yet) possible to access the bin masking information in Python.
   Please check that the correct bins are grayed out in the data view.

.. categories::

.. sourcelink::
