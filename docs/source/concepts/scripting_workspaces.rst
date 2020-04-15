.. _scripting_workspaces:

=====================
Workspaces in Scripts
=====================


The :ref:`Workspaces Toolbox<WorkbenchWorkspaceToolbox>` is used to store and manage workspaces within Mantid. Behind the scences, the Analysis Data Service or ADS is used to control these workspaces. 

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

    wsGroup = GroupWorkspaces("ws1,ws2,ws3")

:ref:`Save a Workspace in a supported ASCII format <algm-SaveAscii>`:

.. code-block:: python

    SaveAscii(InputWorkspace=ws1,Filename=savefile,Separator="Comma")

:ref:`Save a Workspace in Nexus format <algm-SaveNexus>`:

.. code-block:: python

    SaveNexus(InputWorkspace=ws1,Filename=savefile.nxs)

:ref:`Plot Spectra from a Workspace <mantidplot.plotSpectrum>`:

.. code-block:: python

    plotSpectrum(ws,spectrum_nums=[1,2,3],error_bars=True, waterfall=False)



Workspace Management
====================

Required import:

.. code-block:: python

    from mantid.api import AnalysisDataService as ADS
    #to skip this import, use 'mtd' instead of 'ADS'

Access a workspace, loaded in the Workspaces Toolbox, inside a script:

.. code-block:: python

    ws = ADS.retrieve('ws')

Access ALL workspaces,loaded in the Workspaces Toolbox, inside a script:

.. code-block:: python

    ADS.importAll()

Get a list of currently loaded workspaces:

.. code-block:: python

    ws_names = ADS.getObjectNames()

Delete all Workspaces:

.. code-block:: python

    ADS.clear('ws')


Top Tip
=======

*Unsure how to script a certain process*?

* Run the algorithm you want, maybe by clicking the appropriate button in the Workspaces Toolbox (Load, Save...)
* Right-click on the output workspace > Show History
* Save a script for this process to File or Clipboard


Useful links
============

* :ref:`WorkingWithWorkspaces`
* :ref:`More ADS options <mantid.api.AnalysisDataServiceImpl>`
* :ref:`Analysis Data Service Explained <Analysis Data Service>` 
