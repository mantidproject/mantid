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
- ``SetError="custom"`` will recalculate the uncertainties so that each error value matching the ``IfEqualTo`` parameter is replaced with the value in the ``SetErrorTo`` parameter. Additionally, the ``Precision`` parameter lets the user specify how many decimal places of the value in ``IfEqualTo`` need to be matched to be replaced. For example, ``SetErrorTo = 1``, ``IfEqualTo = 0.025`` and ``Precision = 3`` would truncate 0.025123, 0.02599999 etc. to 0.025 and then convert those error values to 1 (but not, for example 0.02499 or 0.026).

The result is a ``Workspace2D`` (:py:obj:`mantid.api.MatrixWorkspace`).

.. categories::

.. sourcelink::
