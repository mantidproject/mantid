
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm finds a so-called most-likely mean of an array being the element that has the minimum of the summed square-rooted absolute distances with all the other elements in the array:

.. math:: MostLikelyMean(a) = a[minindex_{j}\sum_{i} \sqrt{|a_{i} - a_{j}|}]

Usage
-----

**Example - MostLikelyMean**

.. testcode:: MostLikelyMeanExample

   import numpy
   array = numpy.arange(100)
   mlm = MostLikelyMean(array)
   print(mlm)

Output:

.. testoutput:: MostLikelyMeanExample

	49.0

.. categories::

.. sourcelink::
