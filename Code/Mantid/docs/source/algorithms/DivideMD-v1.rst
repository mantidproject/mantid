.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Divide two :ref:`MDHistoWorkspace <MDHistoWorkspace>`'s or a
MDHistoWorkspace and a scalar.

The error of :math:`f = a / b` is propagated with
:math:`df^2 = f^2 * (da^2 / a^2 + db^2 / b^2)`

-  **MDHistoWorkspace / MDHistoWorkspace**

   -  The operation is performed element-by-element.

-  **MDHistoWorkspace / Scalar**

   -  Every element of the MDHistoWorkspace is divided by the scalar.

-  **Scalar / MDHistoWorkspace**

   -  This is not allowed.

-  **`MDEventWorkspace <http://www.mantidproject.org/MDEventWorkspace>`_'s**

   -  This operation is not supported, as it is not clear what its
      meaning would be.

.. categories::
