
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm masks the bins in ``InputWorkspace`` which lie in :math:`X` range that is not covered by ``ComparisonWorkspace``. The ``MaskPartiallyOverlapping`` flag affect the behavior with regards to bins which are partially covered by ``ComparisonWorkspace``.

The algorithm currently applies the default masking weight to the bins which does not clear the data.

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

