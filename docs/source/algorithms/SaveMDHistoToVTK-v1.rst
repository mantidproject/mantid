
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This will save a 3 dimensional :ref:`MDHistoWorkspace` to a VTK file (``.vtu``, StructuredGrid XML VTK file) so that it can be visualized by software such as Paraview.

If the workspace is in HKL space then the correct non-orthogonal transform will be included. To make use of the non-orthogonal transform in Paraview it is required to load the NonOrthogonalSource plugin. To set this plugin navigate to `Tools > Manage Plugins...` to open the Plugin Manager. Enable the Auto Load option on the plugin and press Load Selected.

Usage
-----

.. testcode:: SaveMDHistoToVTK

    import os
    signalInput = [i for i in range(1,28)]
    errorInput = [1 for i in range(1,28)]

    ws = CreateMDHistoWorkspace(SignalInput=signalInput, ErrorInput=errorInput, Dimensionality='3',
                                Extents='-1,1,-1,1,-1,1', NumberOfBins='3,3,3', Names='A,B,C', Units='U,T,W')

    savefile = os.path.join(config["defaultsave.directory"], "mdhws.vts")
    SaveMDHistoToVTK(InputWorkspace = ws, Filename = savefile)

    print("File created: {}".format(os.path.exists(savefile)))

.. testoutput:: SaveMDHistoToVTK

    File created: True

.. testcleanup:: SaveMDHistoToVTK

    os.remove(savefile)

.. categories::

.. sourcelink::
