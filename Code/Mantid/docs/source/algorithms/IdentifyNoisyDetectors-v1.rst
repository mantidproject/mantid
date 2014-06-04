.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The process for this algorithm is:

-  The standard deviation for each pixel within the pre-set range is
   calculated.
-  The mean value and standard deviation of these standard deviation
   values is calculated.
-  Any detector/pixel for which it's standard deviation value satisfied
   the following conditions:

   -  sdev(pixel) < mean(sdevs) - 3 \* sdev(sdevs)
   -  sdev(pixel) > mean(sdevs) + 3 \* sdev(sdevs)
   -  sdev(pixel) < mean(sdevs) \* 0.0001

-  (cont) is considered to be "noisy".

This is repeated three times from the second step.

This uses the :ref:`algm-Integration`, :ref:`algm-Power` and
:ref:`algm-Divide` algorithms for the first step.

The lower bound for the integration is currently fixed to 2000.

The upper bound for the integration is currently fixed to 19000.

.. categories::
