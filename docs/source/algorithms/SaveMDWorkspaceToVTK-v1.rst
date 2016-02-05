.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------
The algorithm SaveMDWorkspaceToVTK will write an IMDHistoWorkspace or IMDEventWorkspace
to either a .vts or .vtu file, respectively.This file can be directly loaded into a
standalone ParaView application. 

To make use of all stored out features, such as axes annotations, it is required to load
the NonOrthogonalSource plugin. To set this plugin navigate to Tools > Manage Plugins and
open the Plugin Manager. Enable the `Auto Load` option on the and press `Load Selected`.

Note that it is currently not possible to save out workspaces with more than three dimensions.


Usage
-----

.. testcode::

  import os
  signalInput = [i for i in range(1,28)]
  errorInput = [1 for i in range(1,28)]

  # Create a sample workspace
  ws = CreateMDHistoWorkspace(SignalInput=signalInput, ErrorInput=errorInput, Dimensionality='3',
                            Extents='-1,1,-1,1,-1,1', NumberOfBins='3,3,3', Names='A,B,C', Units='U,T,W')
  # Save the file out 
  savefile = os.path.join(config["default.savedirectory"], "mdhws")
  SaveMDWorkspaceToVTK(InputWorkspace = ws, Filename = savefile)

.. testcleanup::

  os.remove(filePath)
  
.. categories::

.. sourcelink::
