.. _02_scripting_workspaces:

=====================
Workspaces in Scripts
=====================


The :ref:`Workspaces Toolbox<WorkbenchWorkspaceToolbox>` is used to store and manage workspaces within Mantid. Behind the scenes, the Analysis Data Service or ADS is used to control these workspaces.

Below are some examples of how to control workspaces with a script.

.. contents:: Table of contents
    :local:



Relevant Algorithms
===================

Required import:

.. code-block:: python

    from mantid.simpleapi import *

:ref:`Load a Workspace <algm-Load>` from a file:

.. code-block:: python

    ws = Load('EMU00020884.nxs')

:ref:`Create a Workspace <algm-CreateWorkspace>`:

.. code-block:: python

    ws = CreateWorkspace(DataX=X, DataY=Y, DataE=E, NSpec=4, UnitX="Wavelength")

:ref:`Delete a Workspace <algm-DeleteWorkspace>`:

.. code-block:: python

    DeleteWorkspace(ws)

:ref:`Group Workspaces <WorkspaceGroup>`:

.. code-block:: python

    ws1 = CreateSampleWorkspace()
    ws2 = CreateSampleWorkspace()
    ws3 = CreateSampleWorkspace()
    wsGroup = GroupWorkspaces([ws1,ws2,ws3])

    # or if you only have the names of the workspaces
    wsGroup = GroupWorkspaces(['ws1','ws2','ws3'])

:ref:`Save a Workspace in a supported ASCII format <algm-SaveAscii>`:

.. code-block:: python

    SaveAscii(InputWorkspace=ws,Filename=savefile,Separator="CSV")

:ref:`Save a Workspace in Nexus format <algm-SaveNexus>`:

.. code-block:: python

    SaveNexus(InputWorkspace=ws,Filename=savefile.nxs)

Plot Spectra from a Workspace:

.. code-block:: python

    # simple plot of 1 spectrum
    plotSpectrum(ws, 0)

    # Plot of 3 spectra, and error bars
    plotSpectrum(ws, [0,1,2], error_bars=True)

    # Waterfall plot from two workspaces, with two spectra from each
    plotSpectrum([ws1, ws2], [0,1], waterfall=True)

Above the selected spectra are called by their Workspace Index. Within Workbench, you may wish to call them by their Spectrum Number:

.. code-block:: python

    plotSpectrum(ws1,spectrum_nums=[1,2,3],error_bars=True, waterfall=True)

:ref:`Explanation of the difference between Workspace Index and Spectrum Number <02_the_matrix_workspace>`


Workspace Management
====================

Required import:

.. code-block:: python

    from mantid.api import AnalysisDataService as ADS
    # to skip this import, use 'mtd' instead of 'ADS'

Access a workspace, loaded in the Workspaces Toolbox, inside a script:

.. code-block:: python

    ws = ADS.retrieve('ws')

Access ALL workspaces,loaded in the Workspaces Toolbox, inside a script:

.. code-block:: python

    # This will create a python variable matching the workspace name for each loaded workspace
    ADS.importAll()

Get a list of currently loaded workspaces:

.. code-block:: python

    ws_names = ADS.getObjectNames()

Delete all Workspaces:

.. code-block:: python

    ADS.clear()


Top Tip
=======

*Unsure how to script a certain process*?

* Run the algorithm you want, maybe by clicking the appropriate button in the Workspaces Toolbox (e.g. Load)
* Right-click on the output workspace > Show History
* Save a script for this process to File or Clipboard


Useful links
============

* :ref:`WorkingWithWorkspaces`
* :ref:`More ADS options <mantid.api.AnalysisDataServiceImpl>`
* :ref:`Analysis Data Service Explained <Analysis Data Service>`


Overall Example
===============

.. testcode:: mask_detectors

    from mantid.simpleapi import *
    from mantid.api import AnalysisDataService as ADS
    from mantid.plots._compatability import plotSpectrum #import needed outside Workbench

    ws = CreateSampleWorkspace(); print('Create Workspace')
    print('Workspace list:',ADS.getObjectNames())
    DeleteWorkspace(ws); print('Delete Workspace')
    print('Workspace list:',ADS.getObjectNames())

    ws1 = CreateSampleWorkspace()
    ws2 = CreateSampleWorkspace()
    ws3 = CreateSampleWorkspace()
    wsGroup = GroupWorkspaces([ws1,ws2,ws3]); print('Create + GroupWorkspaces')
    print('Workspace list:',ADS.getObjectNames())

    import os
    filePath = os.path.expanduser('~/SavedNexusFile.nxs')
    SaveNexus(wsGroup,filePath); print('Save Workspaces')

    ADS.clear(); print('Clear All Workspaces')
    print('Workspace list:',ADS.getObjectNames())

    Load(filePath,OutputWorkspace='Saved_wsGroup'); print('Load Data')
    data = ADS.retrieve('Saved_wsGroup')
    print('Workspace list:',ADS.getObjectNames())

    plotSpectrum(ws1,spectrum_nums=[1,2,3],error_bars=True, waterfall=True)

Output:

.. testoutput:: mask_detectors

    Create Workspace
    Workspace list: ['ws']
    Delete Workspace
    Workspace list: []
    Create + GroupWorkspaces
    Workspace list: ['ws1', 'ws2', 'ws3', 'wsGroup']
    Save Workspaces
    Clear All Workspaces
    Workspace list: []
    Load Data
    Workspace list: ['Saved_wsGroup', 'ws1', 'ws2', 'ws3']

.. testcleanup:: mask_detectors

    os.remove(filePath)

.. plot::

    # import mantid algorithms, matplotlib and plotSpectrum
    from mantid.simpleapi import *
    import matplotlib.pyplot as plt
    from mantid.plots._compatability import plotSpectrum #import needed outside Workbench

    ws1 = CreateSampleWorkspace()

    # Plot index 1,2 and 3 from ws1, with errorbars and will a waterfall offset
    plotSpectrum(ws1,spectrum_nums=[1,2,3],error_bars=True, waterfall=True)


.. categories:: Concepts
