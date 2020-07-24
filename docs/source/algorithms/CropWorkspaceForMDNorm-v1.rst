
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm is part of the new workflow for :ref:`normalizing <MDNorm>` multi-dimensional event workspaces.
It is intended to :ref:`crop <algm-CropWorkspace>` the input event workspace, and store the end
of detector trajectories in either momentum (diffraction) or energy transfer (inelastic) units.

**Example - CropWorkspaceForMDNorm**

.. testcode:: CropWorkspaceForMDNormExample

  ws_in = CreateSampleWorkspace(WorkspaceType='Event',
                                Function='Flat background',
                                XUnit='Momentum',
                                XMax=10,
                                BinWidth=0.1)
  ws_out = CropWorkspaceForMDNorm(InputWorkspace=ws_in,
                                  XMin=1,
                                  XMax=6)
  print(f"Number of events in the original workspace {ws_in.getNumberEvents()}")
  print(f"Number of events in the cropped workspace {ws_out.getNumberEvents()}")
  print(f"Largest momentum in the output workspace {ws_out.getSpectrum(1).getTofs().max():.1f}")

.. testoutput:: CropWorkspaceForMDNormExample

  Number of events in the original workspace 200000
  Number of events in the cropped workspace 100000
  Largest momentum in the output workspace 6.0


.. categories::

.. sourcelink::
