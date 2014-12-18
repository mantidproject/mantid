.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This executes the natural logarithm operation on a :ref:`MDHistoWorkspace <MDHistoWorkspace>`.

The signal :math:`a` becomes :math:`f = log(a)`

The error :math:`da` becomes :math:`df^2 = a^2 / da^2`

This algorithm cannot be run on a
`MDEventWorkspace <http://www.mantidproject.org/MDEventWorkspace>`__. Its equivalent on a
:ref:`MatrixWorkspace <MatrixWorkspace>` is called
:ref:`algm-Logarithm`.

.. categories::
