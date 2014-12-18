.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This executes the power function on a :ref:`MDHistoWorkspace <MDHistoWorkspace>`.

The signal :math:`a` becomes :math:`f = a^b`

The error :math:`da` becomes :math:`df^2 = f^2 * b^2 * (da^2 / a^2)`

This algorithm cannot be run on a
`MDEventWorkspace <http://www.mantidproject.org/MDEventWorkspace>`__. Its equivalent on a
:ref:`MatrixWorkspace <MatrixWorkspace>` is called :ref:`algm-Power`.

.. categories::
