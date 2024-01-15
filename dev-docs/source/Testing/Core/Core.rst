.. _core_testing:

============
Core Testing
============

These tests are performed immediately after code-freeze in preparation for a release.

The purpose of these tests is to highlight issues that the individual tests of PRs might have overlooked, or side effects to previous functionality that may have gone undetected. While performing these tests you should also report areas of poor usability and cosmetic defects.


Rules
-----

* The tests must be performed on the installer versions of the final release candidate. Not on local compiled code.
* Serious errors involving loss of functionality, crashes, regression etc. should be raised as ``High Priority`` tickets to the current iteration and an email sent to the project manager immediately.
* Minor and cosmetic issues should be raised as tickets against the next development sprint.
* First try things that should work, then try to break Mantid, e.g. entering invalid values, unexpected characters etc.

Environments
------------
The following environments must be tested, ideally at all host facilities.

- Windows 10 64bit
- Red Hat Enterprise 6 & 7  (IDAaaS at ISIS)
- Ubuntu 18.04
- Mac OS X Catalina

Grouped Tasks
=============

In order to test a wide range of functionality and avoid repeating work, areas to test are split into groups. These tasks are for testing how Workbench links to the Framework, through the different :ref:`Toolboxes <main_window>`. The links here are to give you an idea, but please mess around a little too and try to break Mantid!

This was last updated for Release 6.2. Check if the organiser has considered if the Core testers can test on different environments.



.. csv-table:: Smaller tasks
    :widths: 30 70
    :header: "Area", "Description and Links"

    Install/Uninstall, "**Windows only**: install Mantid, go to the Windows 'Apps & features' list and verify the icon is there for Mantid. Uninstall Mantid from within the 'Apps & features' list and verify that the desktop and start menu shortcuts are removed."
    Project Save/Load, "Test the small areas below and do not delete workspaces or plots you produce and manipulate, maybe open a few interfaces. Then File > Save Project and try to reload it! Diagnose any problems."
    About / First Time Setup Menu, "Under Help > About (**On MAC** Mantid > About). Check Opens Successfully + all buttons and links work"
    Help Documentation, "Open all `Help` menu bar options. Load a few algorithm dialogs and click the **?**. Produce some plots (1D, Waterfall,Colorfill,3D Surface) and check **?** links."
    SliceViewer, "Please **only** test basic SliceViewer functionality, such as outlined :ref:`here <04_displaying_2D_data>`, and please use SV with unusual data and in unusual ways! If you want extra inspiration, here are the advanced :ref:`SliceViewer Manual Testing instructions <sliceviewer_testing>`. Do NOT complete these advanced tests, however you may find the initial Data section useful."
    GUI Plotting, "Produce 1D (Individual, Waterfall, Tiled), 2D (Colorfill and Contour) and 3D (Surface and Wireframe) plots with different data types. :ref:`03_displaying_1D_data` :ref:`3D_Plots`"
    Manipulating Plots, "See Plot Help Docs for info on Toolbar **?** buttons, Click Menus, Plots Toolbox, Settings"
    Plot Workspace Rename, "Load some data, make a plot, rename the workspace, check the window and plot titles have updated"
    Scripting Plots, "For some ideas: :ref:`01_basic_plot_scripting` :ref:`06_formatting_plots` :ref:`plotting` + run any `MPL code <https://matplotlib.org/gallery/index.html>`_ and check if Figure Options and Generate a script work."
    Settings (Preferences on Mac), "Check all options work. Some may need restart."
    Script Editor / Interpreter, "Check basic functionality, such as checking the buttons/options at the top of SE, running algorithms, accessing workspace properties in both environments. Also, check that closing a tab, while it's script is running, aborts the script properly. Does autocompletion work for mantid algorithms/numpy/matplotlib? :ref:`02_scripts`"
    Access mantid framework in Python and Jupyter Notebook, "Follow the instructions on :ref:`PythonNotebook` and check you can import mantid.simpleapi and run a script/notebook."
    System Memory Usage Widget, "Check that the System Memory Usage Widget updates at a frequency of no more than once every two seconds, ideally on a system with limited resourses. Check that the default position of the widget is in the top right, in the same column as the Messages widget."
    Script Repository, "Test downloading and uploading scripts to the Script Repository. Try to move the Script Repository to another folder. Check out :ref:`this <WorkbenchScriptRepository>` page for more information."

.. csv-table:: Larger tasks
    :widths: 10 20 70
    :header: "Area", Sub-section, "Description and Links"

    **Data and Workspace Menus**
    ,Data Loading, "Load different data types from different facilities"
    ,Save Nexus/Ascii, "Save a few appropriate workspaces (both the algorithm and from the Save button at top of Workspaces Toolbox)"
    ,Show Data, "Display the Data for different workspaces: Workspace2D (Histogram), EventWorkspace, TableWorkspace AND plotBin and plotSpectrum from the Data table"
    ,Instrument Viewer, "Open Instrument viewer for instruments in different facilities."
    ,SliceViewer, "Open and make some small changes (full test is separate)"
    ,Show Detectors,
    ,Sample Logs,
    ,Workspace History, "Check Script generation from History works in various cases"
    **Running Script**
    ,Random scripts, "e.g. from Solutions to last 2 induction courses, Script Repo"
    ,Workspace Algebra, "See bottom of this page: :ref:`MatrixWorkspace`"
    ,Error handling in Python, "Get creative, e.g. run code without imports, wrong filepath in Manage User directories, indent errors. Are Error Messages Useful??"
    **Algorithms**
    ,Execution, "Check 5-10 algorithms"
    ,Input Validation, "Input invalid values and see if a USEFUL `*` tooltip appears or after executing there is a useful error"
    ,Progress and Cancelling, "Show Algorithm Toolbox + run some algorithms. Is the Progress Reported helpfully, under Details do algorithms appear and does cancelling work? e.g. Pause"
    **Fitting**
    ,Normal, "Plot a spectrum, click 'Fit' Toolbar button. Add different functions, add ties. :ref:`02_complex_case`"
    ,User Defined Function, "Halfway down this page: :ref:`03_fit_model_choices`"
    ,Sequential, "This uses the :ref:`algm-PlotPeakByLogValue` algorithm. Load a workspace with multiple spectra. Plot one spectrum and click the 'Fit' Toolbar button. Add an appropriate fit function and click Fit>Sequential Fit. If only one ws is selelcted then all spectra are fit. Try with a range of spectra (e.g. 1:10). Fit spectra from a WorkspaceGroup."
    ,Fit Algorithm, "See :ref:`algm-Fit`, check ties work"
