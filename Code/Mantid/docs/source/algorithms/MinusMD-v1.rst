.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Subtract two `MDHistoWorkspace <MDHistoWorkspace>`__'s or a
MDHistoWorkspace and a scalar.

-  **MDHistoWorkspace - MDHistoWorkspace**

   -  The operation is performed element-by-element.

-  '''MDHistoWorkspace - Scalar '''

   -  The scalar is subtracted from every element of the
      MDHistoWorkspace. The squares of errors are summed.

-  **Scalar - MDHistoWorkspace**

   -  This is not allowed.

-  **`MDEventWorkspace <MDEventWorkspace>`__ -
   `MDEventWorkspace <MDEventWorkspace>`__**

   -  The signal of each event on the right-hand-side is multiplied by
      -1 before the events are summed.
   -  The number of events in the output MDEventWorkspace is that of the
      LHS and RHS workspaces put together.

-  **`MDEventWorkspace <MDEventWorkspace>`__ - Scalar or
   MDHistoWorkspace**

   -  This is not possible.

.. categories::
