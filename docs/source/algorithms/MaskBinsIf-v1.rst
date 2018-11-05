
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm masks bins according to the criteria specified as a `muparser <http://beltoforion.de/article.php?a=muparser>`_ expression.
The variables entering the criteria are reserved as follows:

- y : count in a bin (arbitrary units)
- x : the bin center (arbitrary units)
- e : the standard deviation on the count
- dx : the error on the bin center
- s : the value of spectrum axis, which has to be SpectraAxis or NumericAxis.

Note that it preserves the data in the masked bins, just flags them as masked.

Usage
-----

**Example - MaskBinsIf**

.. code-block:: python

   CreateSampleWorkspace(BankPixelWidth=100, NumBanks=1, OutputWorkspace='out')
   MaskBinsIf(InputWorkspace='out', Criterion='s>10 && s<20 && x>1000 && x<2000', OutputWorkspace='out')

.. categories::

.. sourcelink::
