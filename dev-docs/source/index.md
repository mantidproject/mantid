# Developer Documentation

These pages contain the developer documentation for mantid. They are
aimed at those who are modifying the source code of the project. For
user documentation please see `here <mantid:contents>`.

# Guides

::: {.toctree hidden=""}
DeveloperAccounts GettingStarted/GettingStarted Packaging Architecture
BuildingOnOSX BuildingWithCMake LocalMantidBuildWithPixi
CMakeBestPractices Standards/index Testing/index DoxygenSetup
NewStarterC++ NewStarterPython PythonAlgorithmsInExternalProjects
:::

`DeveloperAccounts`

:   Details of the accounts required for developers.

`Getting Started <GettingStarted/GettingStarted>`

:   Describes the process of obtaining and building the mantid code
    base.

`Packaging`

:   Describes the process building the user packages.

`Architecture`

:   Describes the architecture of the mantid libraries and applications.

`LocalMantidBuildWithPixi`

:   How to set up a pixi environment to use a local Mantid build instead
    of conda packages in other projects.

`Standards <Standards/index>`

:   Details of coding and documentation standards for the project.
    Includes specifics regarding algorithms.

`Manual Testing <Testing/index>`

:   Details of manual and acceptance/smoke testing.

`DoxygenSetup`

:   Configure a doxygen build locally.

`NewStarterC++`

:   Exercises for learning C++.

`NewStarterPython`

:   Exercises for learning Python.

# Development Process

::: {.toctree hidden=""}
DevelopmentAndReleaseCycle Communication IssueTracking UserSupport
GitConfig GitWorkflow AutomatedBuildProcess JenkinsConfiguration
ReleaseChecklist PatchReleaseChecklist TSC DesignDocumentGuides
CodeCoverage
:::

`DevelopmentAndReleaseCycle`

:   An overview of the development cycle.

`Communication`

:   Details various methods of communication used within the team.

`IssueTracking`

:   Describes how issues are tracked over the project.

`UserSupport`

:   Procedures for User Problems to be tested and passed to the
    Development Team.

`GitWorkflow`

:   Details the workflow used development with git and GitHub.

`AutomatedBuildProcess`

:   Details the interaction of pull requests with the Jenkins CI builds.

`JenkinsConfiguration`

:   Describes the setup of Jenkins system and how to add a new slave.

`ReleaseChecklist`

:   How to perform a full release of Mantid.

`PatchReleaseChecklist`

:   How to perform a patch release of Mantid.

`TSC`

:   Overview of the role of the technical steering committee.

`DesignDocumentGuides`

:   When and how to write a good design document.

# Tools

::: {.toctree hidden=""}
ToolsOverview ProfilingOverview Timers ProfilingWithPerf
ProfilingWithJemalloc ProfilingWithValgrind FlowchartCreation
VisualStudioBuildImpact PyCharm CLion VSCode Eclipse
WindowsSubsystemForLinux ObtainingABenchmarkForMantidFitting
CondaPackageManager
:::

`ToolsOverview`

:   Describes `class_maker`, `valgrind` and related tools.

`ProfilingOverview`

:   There are a few different ways to profile Mantid code.

`Timers`

:   Describes different ways of timing Mantid C++ code.

`FlowchartCreation`

:   Describes how to create a flow chart with dot.

`VisualStudioBuildImpact`

:   Provides a script to reduce the impact of Visual Studio on machine
    performance.

`PyCharm`

:   Describes how to set up the PyCharm interpreter, and debug python
    code (Windows/Linux only).

`CLion`

:   Describes how to set up CLion to build and debug using a Ninja
    generator (Windows/Linux only).

`VSCode`

:   Guide to using VSCode for C++ with Mantid.

`Eclipse`

:   Guide to setting up Eclipse on Ubuntu

`WindowsSubsystemForLinux`

:   Guide for setting up Ubuntu 18.04 and Centos7 as subsystems on
    Windows (WSL2).

`ObtainingABenchmarkForMantidFitting`

:   Guide for setting up an environment to perform a benchmark of Mantid
    fitting minimizers.

`CondaPackageManager`

:   Guide on how to use the conda package manager in Mantid, including
    tips and a `pip` policy.

# Testing

::: {.toctree hidden=""}
RunningTheUnitTests DebuggingUnitTests UnitTestGoodPractice
ReviewingAPullRequest Gatekeeping WritingPerformanceTests SystemTests
DataFilesForTesting TestingUtilities RunningSanitizers UnittestMonitor
:::

`RunningTheUnitTests`

:   Details on how to run the suite of unit tests.

`DebuggingUnitTests`

:   Details on how to debug the suite of unit tests.

`UnitTestGoodPractice`

:   Guidance on writing good unit tests.

`ReviewingAPullRequest`

:   What to do when reviewing an individual contribution to mantid.

`Gatekeeping`

:   Things to consider when merging a pull request to the main
    production branch.

`WritingPerformanceTests`

:   A walk through of how to write a performance test.

`SystemTests`

:   Guidance on working with the system tests.

`DataFilesForTesting`

:   How to work with test data files in the mantid repository.

`TestingUtilities`

:   Helper utlities used for testing.

`RunningSanitizers`

:   How to run the various sanitizers locally.

`UnittestMonitor`

:   Monitor failing and flakey unittests from the Jenkins nightly
    pipelines.

# GUI Development

::: {.toctree hidden=""}
MVPDesign MVPTutorial/index QtDesignerForPython BalsamiqWireframes
MantidUsedIconsTable ISISReflectometryInterface
:::

`MVPDesign`

:   Gives some guidelines to consider when developing a new graphical
    user interface.

`MVP Tutorial <MVPTutorial/index>`

:   A hands-on tutorial on how to implement a user GUI using the
    Model-View-Presenter (MVP) pattern.

`QtDesignerForPython`

:   Describes how to use the Qt designer to produce GUI views.

`BalsamiqWireframes`

:   An introduction to mockups with Balsamiq Wireframes.

`MantidUsedIconsTable`

:   The currently used Icons in Mantid and what they are used for.

`ISISReflectometryInterface`

:   An example of a complex C++ interface that uses MVP.

# Workbench

The workbench is the new PyQt-based GUI that will be the primary
interface for interacting with the mantid framework. The plotting is
provided by [matplotlib](https://matplotlib.org/). It has replaced
MantidPlot.

::: {.toctree hidden=""}
Workbench/index
:::

`Workbench Documentation <Workbench/index>`

:   The Index of the workbench specific files.

# Component Overviews

::: {.toctree maxdepth="1"}
BatchWidget/index EnumeratedString EnumeratedStringProperty
EventWorkspaceDev HandlingXML IndexProperty
IndirectDataAnalysisAddingFitType IndirectDataAnalysisFileStructure
InelasticDataProcessorFileStructure InstrumentViewer
ISISReflectometryInterface ISISSANSReductionBackend LoadAlgorithmHook
Logging MatplotlibInCpp MultiThreadingInMantid PythonVSCppAlgorithms
SampleLogsDev ScriptRepository Widgets/Plotting WritingAnAlgorithm
DynamicProperties WritingCustomConvertToMDTransformation
ISISEnergyTransferTab
:::
