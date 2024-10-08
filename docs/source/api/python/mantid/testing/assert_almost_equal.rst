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
If neither ``rtol`` nor ``atol``  is specified, will perform an absolute comparison to within 1.e-10.

.. module:`mantid.testing`

.. automodule:: mantid.testing.assert_almost_equal
    :members:
    :undoc-members:
    :inherited-members:

Usage
-----

**Example - Comparison that passes**

.. testcode:: ExAssertAlmostEqualPasses

    from mantid.testing import assert_almost_equal
    ts1 = CreateSingleValuedWorkspace(0.1)
    ts2 = CreateSingleValuedWorkspace(0.2)
    assert_almost_equal(ts1, ts2, atol=0.1)  # passes

.. testcleanup:: ExAssertAlmostEqualPasses

   DeleteWorkspace(ts1)
   DeleteWorkspace(ts2)

**Example - Comparison that fails**

.. testcode:: ExAssertNotEqualFails

    from mantid.testing import assert_not_equal
    ts1 = CreateSingleValuedWorkspace(0.1)
    ts2 = CreateSingleValuedWorkspace(0.2)
    # uncomment to see that an AssertionError is raised
    # assert_almost_equal(ts1, ts2, atol=0.01)

.. testcleanup:: ExAssertNotEqualFails

   DeleteWorkspace(ts1)
   DeleteWorkspace(ts2)

See Also
--------
:ref:`algm-CompareWorkspaces`, :ref:`mantid.testing.assert_not_equal`