.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This executes the natural logarithm operation on a :py:obj:`MDHistoWorkspace <mantid.dataobjects.MDHistoWorkspace>`.

The signal :math:`a` becomes :math:`f = log(a)`

The error :math:`da` becomes :math:`df^2 = a^2 / da^2`

This algorithm cannot be run on a
:py:obj:`MDEventWorkspace <mantid.api.IMDWorkspace>`. Its equivalent on a
:py:obj:`MatrixWorkspace <mantid.api.MatrixWorkspace>` is called
:ref:`algm-Logarithm`.

.. categories::

.. sourcelink::
