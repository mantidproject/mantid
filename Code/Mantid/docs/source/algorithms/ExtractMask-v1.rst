.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The masking from the InputWorkspace property is extracted by creating a
new MatrixWorkspace with a single X bin where:

-  0 = masked;
-  1 = unmasked.

The spectra containing 0 are also marked as masked and the instrument
link is preserved so that the instrument view functions correctly.

.. categories::
