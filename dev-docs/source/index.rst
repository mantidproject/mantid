.. _contents:

=======================
Developer Documentation
=======================

.. toctree::
   :hidden:

   DevelopmentTeam

These pages contain the developer documentation for mantid. They are aimed at those who are modifying the
source code of the project. For user documentation please see :ref:`here <mantid:contents>`.

Meet the :ref:`team <DevelopmentTeam>`.

======
Guides
======

.. toctree::
   :hidden:

   DeveloperAccounts
   GettingStarted
   BuildingOnOSX
   BuildingWithCMake
   BuildingVATES
   Standards/index
   DoxygenSetup
   Python3

:doc:`DeveloperAccounts`
   Details of the accounts required for developers.

:doc:`GettingStarted`
   Describes the process of obtaining and building the mantid code base.

:doc:`Standards <Standards/index>`
   Details of coding and documentation standards for the project. Includes specifics regarding algorithms.

:doc:`DoxygenSetup`
   Configure a doxygen build locally.

:doc:`Python3`
   Building with Python 3 (Linux only).

===================
Development Process
===================

.. toctree::
   :hidden:

   DevelopmentAndReleaseCycle
   Communication
   IssueTracking
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
   ProfilingWithValgrind
   FlowchartCreation
   VisualStudioBuildImpact
   GettingStartedWithPyCharm
   Eclipse

:doc:`ToolsOverview`
   Describes ``class_maker``, ``valgrind`` and related tools.

:doc:`ProfilingWithValgrind`
   How to use valgrind to profile your code.

:doc:`FlowchartCreation`
   Describes how to create a flow chart with dot.

:doc:`VisualStudioBuildImpact`
   Provides a script to reduce the impact of Visual Studio on machine performance.

:doc:`GettingStartedWithPyCharm`
   Describes how to set up the PyCharm interpreter, and debug python code (Windows/Linux only).
   
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
   WritingPerformanceTests
   SystemTests
   DataFilesForTesting
   TestingUtilities

:doc:`RunningTheUnitTests`
   Details on how to run the suite of unit tests.

:doc:`DebuggingUnitTests`
   Details on how to debug the suite of unit tests.

:doc:`UnitTestGoodPractice`
   Guidance on writing good unit tests.

:doc:`WritingPerformanceTests`
   A walk through of how to write a performance test.

:doc:`SystemTests`
   Guidance on working with the system tests.

:doc:`DataFilesForTesting`
   How to work with test data files in the mantid repository.

:doc:`TestingUtilities`
   Helper utlities used for testing.

===============
GUI Development
===============

.. toctree::
   :hidden:

   GUIDesignGuidelines
   MVPTutorial/index
   QtDesignerForPython

:doc:`GUIDesignGuidelines`
   Gives some guidelines to consider when developing a new graphical user interface.

:doc:`MVP Tutorial <MVPTutorial/index>`
   A hands-on tutorial on how to implement a user GUI using the Model-View-Presenter (MVP) pattern.

:doc:`QtDesignerForPython`
   Describes how to use the Qt designer to produce GUI views.

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
   ISISSANSReductionBackend
   LoadAlgorithmHook
   Logging
   MultiThreadingInAlgorithms
   PythonVSCppAlgorithms
   RemoteJobSubmissionAPI
   WritingAnAlgorithm
   WritingCustomConvertToMDTransformation

