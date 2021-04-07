.. _contents:

=======================
Developer Documentation
=======================

These pages contain the developer documentation for mantid. They are aimed at those who are modifying the
source code of the project. For user documentation please see :ref:`here <mantid:contents>`.

======
Guides
======

.. toctree::
   :hidden:

   DeveloperAccounts
   GettingStarted
   BuildingOnOSX
   BuildingWithCMake
   Standards/index
   Testing/index
   DoxygenSetup

:doc:`DeveloperAccounts`
   Details of the accounts required for developers.

:doc:`GettingStarted`
   Describes the process of obtaining and building the mantid code base.

:doc:`Standards <Standards/index>`
   Details of coding and documentation standards for the project. Includes specifics regarding algorithms.

:doc:`Manual Testing <Testing/index>`
   Details of manual and acceptance/smoke testing.

:doc:`DoxygenSetup`
   Configure a doxygen build locally.

`C++ Introduction <https://www.mantidproject.org/New_Starter_C%2B%2B_introduction>`_
   Exercises for learning C++.

===================
Development Process
===================

.. toctree::
   :hidden:

   DevelopmentAndReleaseCycle
   Communication
   IssueTracking
   UserSupport
   GitWorkflow
   AutomatedBuildProcess
   JenkinsConfiguration
   ReleaseChecklist
   PatchReleaseChecklist
   TSC
   DesignDocumentGuides

:doc:`DevelopmentAndReleaseCycle`
   An overview of the development cycle.

:doc:`Communication`
   Details various methods of communication used within the team.

:doc:`IssueTracking`
   Describes how issues are tracked over the project.

:doc:`UserSupport`
   Procedures for User Problems to be tested and passed to the Development Team.

:doc:`GitWorkflow`
   Details the workflow used development with git and GitHub.

:doc:`AutomatedBuildProcess`
   Details the interaction of pull requests with the Jenkins CI builds.

:doc:`JenkinsConfiguration`
   Describes the setup of Jenkins system and how to add a new slave.

:doc:`ReleaseChecklist`
   How to perform a full release of Mantid.

:doc:`PatchReleaseChecklist`
   How to perform a patch release of Mantid.

:doc:`TSC`
   Overview of the role of the technical steering committee.

:doc:`DesignDocumentGuides`
   How to write a good design document.

=====
Tools
=====

.. toctree::
   :hidden:

   ToolsOverview
   ProfilingOverview
   FlowchartCreation
   VisualStudioBuildImpact
   PyCharm
   VSCode
   Eclipse

:doc:`ToolsOverview`
   Describes ``class_maker``, ``valgrind`` and related tools.

:doc:`ProfilingOverview`
   There are a few different ways to profile Mantid code.

:doc:`FlowchartCreation`
   Describes how to create a flow chart with dot.

:doc:`VisualStudioBuildImpact`
   Provides a script to reduce the impact of Visual Studio on machine performance.

:doc:`PyCharm`
   Describes how to set up the PyCharm interpreter, and debug python code (Windows/Linux only).

:doc:`VSCode`
   Guide to using VSCode for C++ with Mantid.

:doc:`Eclipse`
   Guide to setting up Eclipse on Ubuntu

=======
Testing
=======

.. toctree::
   :hidden:

   RunningTheUnitTests
   DebuggingUnitTests
   UnitTestGoodPractice
   ReviewingAPullRequest
   WritingPerformanceTests
   SystemTests
   DataFilesForTesting
   TestingUtilities
   RunningSanitizers

:doc:`RunningTheUnitTests`
   Details on how to run the suite of unit tests.

:doc:`DebuggingUnitTests`
   Details on how to debug the suite of unit tests.

:doc:`UnitTestGoodPractice`
   Guidance on writing good unit tests.

:doc:`ReviewingAPullRequest`
   What to do when reviewing an individual contribution to mantid.

:doc:`WritingPerformanceTests`
   A walk through of how to write a performance test.

:doc:`SystemTests`
   Guidance on working with the system tests.

:doc:`DataFilesForTesting`
   How to work with test data files in the mantid repository.

:doc:`TestingUtilities`
   Helper utlities used for testing.

:doc:`RunningSanitizers`
   How to run the various sanitizers locally.

===============
GUI Development
===============

.. toctree::
   :hidden:

   MVPDesign
   MVPTutorial/index
   QtDesignerForPython
   MantidUsedIconsTable
   ISISReflectometryInterface

:doc:`MVPDesign`
   Gives some guidelines to consider when developing a new graphical user interface.

:doc:`MVP Tutorial <MVPTutorial/index>`
   A hands-on tutorial on how to implement a user GUI using the Model-View-Presenter (MVP) pattern.

:doc:`QtDesignerForPython`
   Describes how to use the Qt designer to produce GUI views.

:doc:`MantidUsedIconsTable`
   The currently used Icons in Mantid and what they are used for.

:doc:`ISISReflectometryInterface`
   An example of a complex C++ interface that uses MVP.

=========
Workbench
=========

The workbench is the new PyQt-based GUI that will be the primary interface for
interacting with the mantid framework. The plotting is provided by
`matplotlib <https://matplotlib.org/>`_. It will eventually replace MantidPlot.

.. toctree::
   :hidden:

   Workbench/index

:doc:`Workbench Documentation <Workbench/index>`
   The Index of the workbench specific files.

===================
Component Overviews
===================

.. toctree::
   :maxdepth: 1

   AlgorithmMPISupport
   BatchWidget/index
   EventWorkspaceDev
   HandlingXML
   IndexProperty
   InstrumentViewer
   ISISReflectometryInterface
   ISISSANSReductionBackend
   LoadAlgorithmHook
   Logging
   MatplotlibInCpp
   MultiThreadingInAlgorithms
   PythonVSCppAlgorithms
   RemoteJobSubmissionAPI
   SampleLogsDev
   Widgets/Plotting
   WritingAnAlgorithm
   WritingCustomConvertToMDTransformation
