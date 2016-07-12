.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The uncertainties for the entire workspace will be recalculated according to the ``SetError`` property.

- ``SetError="zero"`` will change all of the uncertainties to zero
- ``SetError="sqrt"`` will recalculate all of the uncertainties to be the square root of the y value
- ``SetError="oneIfZero"`` will change the uncertainties to one if they are currently zero
- ``SetError="sqrtOrOne"`` will recalculate all of the uncertainties to be the square root of the y value. If the uncertainty is zero it will be set to one.

The result is a ``Workspace2D`` (:py:obj:`mantid.api.MatrixWorkspace`).

.. categories::

.. sourcelink::
