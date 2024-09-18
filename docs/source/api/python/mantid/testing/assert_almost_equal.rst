.. _mantid.testing.assert_almost_equal:

=====================
 assert_almost_equal
=====================

This is a Python function for testing if two modules are within a tolerance.
If ``rtol`` is specified, will compare using the relative difference.
If ``atol`` is specified, will compare using the absolute difference.
One or both of ``rtol`` and ``atol`` may be specified.
Will run the underlying :ref:`algm-CompareWorkspaces` algorithm for each
case specified, and fail if any of the comparisons fail.
If neither ``rtol`` nor ``atol``  is specified, will perform an absolute compatison to within 1.e-10.

.. module:`mantid.testing`

.. automodule:: mantid.testing.assert_almost_equal
    :members:
    :undoc-members:
    :inherited-members:

See Also
--------
:ref:`algm-CompareWorkspaces`
