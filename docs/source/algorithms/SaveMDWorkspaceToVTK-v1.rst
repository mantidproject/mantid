.. algorithm::

.. summary::

.. relatedalgorithms::

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

.. testcode:: SaveMDWorkspaceToVTK
  
    import os
    signalInput = [i for i in range(1,28)]
    errorInput = [1 for i in range(1,28)]
    
    ws = CreateMDHistoWorkspace(SignalInput=signalInput, ErrorInput=errorInput, Dimensionality='3',
                                Extents='-1,1,-1,1,-1,1', NumberOfBins='3,3,3', Names='A,B,C', Units='U,T,W')
                                
    ws2 = CreateMDHistoWorkspace(SignalInput='1,2,3,4,5,6,7,8,9', ErrorInput='1,1,1,1,1,1,1,1,1', Dimensionality='2',
                                Extents='-1,1,-1,1', NumberOfBins='3,3', Names='A,B', Units='U,T')                            
  
    savefile = os.path.join(config["defaultsave.directory"], "mdhws.vts")
    SaveMDWorkspaceToVTK(InputWorkspace = ws, Filename = savefile)

    print("File created: {}".format(os.path.exists(savefile)))

.. testoutput:: SaveMDWorkspaceToVTK

    File created: True


.. testcleanup:: SaveMDWorkspaceToVTK

    os.remove(savefile)
  
.. categories::

.. sourcelink::
