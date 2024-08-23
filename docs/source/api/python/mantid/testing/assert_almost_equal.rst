.. _mantid.testing.assert_almost_equal:

=====================
 assert_almost_equal
=====================

This is a Python function for testing if two modules are within a tolerance.
At least ``rtol`` or ``atol`` parameters should be specified. Neither being
specified generates a ``ValueError`` exception. Both being specified runs
the underlying :ref:`algm-CompareWorkspaces` algorithm
twice, with relative tolerance being first.



.. module:`mantid.testing`

.. automodule:: mantid.testing.assert_almost_equal
    :members:
    :undoc-members:
    :inherited-members:

See Also
--------
:ref:`algm-CompareWorkspaces`
