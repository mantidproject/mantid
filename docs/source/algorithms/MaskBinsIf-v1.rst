
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm masks bins according to the criteria specified as a `muparser <http://beltoforion.de/article.php?a=muparser>`_ expression.
The variables entering the criteria are reserved as follows:

- y : count in a bin
- x : the bin center
- e : the error on the count
- dx : the error on the bin center
- i : the workspace index
- s : the value of spectrum axis, in case of SpectraAxis or NumericAxis.

Usage
-----

**Example - MaskBinsIf**

.. code-block:: python

   CreateSampleWorkspace(BankPixelWidth=100, NumBanks=1, OutputWorkspace='out')
   MaskBinsIf(InputWorkspace='out', Criterion='i >10 && i<20 && x>1000 && x<2000', OutputWorkspace='out')

.. categories::

.. sourcelink::
