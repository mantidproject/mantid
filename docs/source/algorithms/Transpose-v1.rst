.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm transposes a workspace, so that an N1 x N2 workspace
becomes N2 x N1. 

The X-vector values for the new workspace are taken from the axis values
of the old workspace, which is generaly the spectra number but can be
other values, if say the workspace has gone through
:ref:`ConvertSpectrumAxis <algm-ConvertSpectrumAxis>`.

.. warning::

    The new axis values are taken from the previous X-vector values for the
    first specrum in the workspace. For this reason, use with ragged
    workspaces is undefined.
    
For transposing multidimensional workspaces use :ref:`TransposeMD <algm-TransposeMD>`.

Usage
-----

.. testcode:: Ex

    # Create a workspace with 2 banks and 4 pixels per bank (8 spectra)
    # and X axis with 100 bins
    ws = CreateSampleWorkspace(BankPixelWidth=2)
    print("Rank before = ( {0}, {1} )".format(ws.getNumberHistograms(),ws.blocksize()))
    ws = Transpose(ws)
    print("Rank after = ( {0}, {1} )".format(ws.getNumberHistograms(),ws.blocksize()))

Output:

.. testoutput:: Ex

    Rank before = ( 8, 100 )
    Rank after = ( 100, 8 )

.. categories::

.. sourcelink::
      :h: Framework/Algorithms/inc/MantidAlgorithms/Transpose.h
