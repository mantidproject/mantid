.. _mantid.api.AnalysisDataServiceImpl:

=========================
 AnalysisDataServiceImpl
=========================

This is a Python binding to the C++ class Mantid::API::AnalysisDataServiceImpl.

What is it?
-----------

The Analysis Data Service is a :ref:`Data Service <Data Service>` that is
specialized to hold all of the :py:obj:`Workspaces <mantid.api.Workspace>` that are
created by user run :py:obj:`algorithms <mantid.api.Algorithm>`. Whenever an algorithm is
executed it automatically extracts its input workspaces from the
Analysis Data Service, and inserts its output workspaces upon
completion.

Extracting a workspace from the Analysis Data Service
-----------------------------------------------------

The most usual way in user code would be to use the :py:obj:`FrameworkManager <mantid.api.FrameworkManagerImpl>`.

``Workspace* result = FrameworkManager::Instance().getWorkspace("workspaceName")``

Or you could get it directly from the AnalysisDataService (as a :ref:`Shared
Pointer <Shared Pointer>`)

``Workspace_sptr result = AnalysisDataService::Instance().retrieve("test_out1");``

If you were writing an algorithm however you would most likely use a
Workspace :py:obj:`Property <mantid.kernel.Property>` to access or store your workspaces.


Reference
---------

.. module:`mantid.api`

.. autoclass:: mantid.api.AnalysisDataServiceImpl
    :members:
    :undoc-members:
    :inherited-members:
