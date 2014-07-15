.. _Analysis Data Service:

Analysis_Data_Service
=====================

What is it?
-----------

The Analysis Data Service is a `Data Service <Data Service>`__ that is
specialized to hold all of the `workspaces <Workspace>`__ that are
created by user run `algorithms <Algorithm>`__. Whenever an algorithm is
executed it automatically extracts its input workspaces from the
Analysis Data Service, and inserts its output workspaces upon
completion.

Extracting a workspace from the Analysis Data Service
-----------------------------------------------------

The most usual way in user code would be to use the `Framework
Manager <Framework Manager>`__.

``Workspace* result = FrameworkManager::Instance().getWorkspace("workspaceName")``

Or you could get it directly from the AnalysisDataService (as a `Shared
Pointer <Shared Pointer>`__)

``Workspace_sptr result = AnalysisDataService::Instance().retrieve("test_out1");``

If you were writing an algorithm however you would most likely use a
Workspace `Property <Properties>`__ to access or store your workspaces.



.. categories:: Concepts