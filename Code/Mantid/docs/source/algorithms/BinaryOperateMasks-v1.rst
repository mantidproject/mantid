.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

A binary operation will be conducted on two SpecialWorkspace2D (i.e.,
masking workspace). The binary operations supported include AND, OR and
XOR (exclusive or). The operation is done between the corresponding
spectra of these two input workspaces, i.e.,

.. math:: spec_i^{output} = spec_i^{in 1} \times spec_i^{in 2}

.. math:: spec_i^{output} = spec_i^{in 1} + spec_i^{in 2}

.. math:: spec_i^{output} = spec_i^{in 1} \oplus spec_i^{in 2}

Output
------

A SpecialWorkspace2D with the same dimension and geometry as the input
two SpecialWorkspace2D.

.. categories::
