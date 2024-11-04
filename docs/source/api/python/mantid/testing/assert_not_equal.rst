.. _mantid.testing.assert_not_equal:

=====================
 assert_not_equal
=====================

This is a Python function for testing that two workspaces are not equal within a tolerance.
If ``rtol`` is specified, the workspaces will be compared using the relative difference.
If ``atol`` is specified, the workspaces will be compared using the absolute difference.
Only one of ``rtol`` and ``atol`` may be specified.
``assert_not_equal`` will run the underlying :ref:`algm-CompareWorkspaces` algorithm for the
case specified, and fail if the comparison succeeds.
If neither ``rtol`` nor ``atol``  is specified, will perform an absolute comparison to within 1.e-10.

.. module:`mantid.testing`

.. automodule:: mantid.testing.assert_not_equal
    :members:
    :undoc-members:
    :inherited-members:

Usage
-----

**Example - Comparison that passes**

.. testcode:: ExAssertNotEqualPasses

    from mantid.testing import assert_not_equal
    ts1 = CreateSingleValuedWorkspace(0.1)
    ts2 = CreateSingleValuedWorkspace(0.3)
    assert_not_equal(ts1, ts2, atol=0.1)  # passes

.. testcleanup:: ExAssertNotEqualPasses

   DeleteWorkspace(ts1)
   DeleteWorkspace(ts2)

**Example - Comparison that fails**

.. testcode:: ExAssertNotEqualFails

    from mantid.testing import assert_not_equal
    ts1 = CreateSingleValuedWorkspace(0.1)
    ts2 = CreateSingleValuedWorkspace(0.3)
    # uncomment to see that an AssertionError is raised
    # assert_not_equal(ts1, ts2, atol=1.0)

.. testcleanup:: ExAssertNotEqualFails

   DeleteWorkspace(ts1)
   DeleteWorkspace(ts2)

See Also
--------
:ref:`algm-CompareWorkspaces`, :ref:`mantid.testing.assert_almost_equal`
