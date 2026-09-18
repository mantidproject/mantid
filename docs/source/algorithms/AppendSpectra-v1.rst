.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm appends the spectra of two workspaces together.

The output workspace from this algorithm will be a copy of the first
input workspace, to which the data from the second input workspace will
be appended.

Workspace data members other than the data (e.g. instrument etc.) will
be copied from the first input workspace (but if they're not identical
anyway, then you probably shouldn't be using this algorithm!).

Restrictions on the input workspace
###################################

For an :py:obj:`EventWorkspace <mantid.dataobjects.EventWorkspace>`, there are no restrictions on
the input workspaces if ValidateInputs=false.

For :py:obj:`Workspace2D <mantid.dataobjects.Workspace2D>`, the number of bins must be the same
in both inputs.

Mixing workspace types is not allowed.

Additionally, if ValidateInputs is selected, then the input workspaces are also required to:

-  Come from the same instrument
-  Have common units
-  Have common bin boundaries

Spectrum Numbers
################

If there is an overlap in the spectrum numbers of both inputs, then the
output workspace will have its spectrum numbers reset starting at 0 and
increasing by 1 for each spectrum. In addition, the y-axis value will be copied
from previous workspaces in order of the first workspace then the second workspace.

Note that when spectra numbers do not overlap,
it doesn't automatically imply that y-axis values are carried over from previous workspaces.
To address this, use the 'AppendYAxisLabels' option.
This will combine y-axis values from two input workspaces into the new output workspace,
arranging them in the order of the first workspace followed by the second. In addition, the axes
should have the same type.

Rewrite Spectra Map
###################

Some instruments have moveable detector banks and a frequently used workflow is to make two measurements with the detector banks at slightly offset positions
such that gaps between the detector tubes are covered. AppendSpectra does combine the two runs only using the detector map from the first run.
Thus spectra from the second run although having the correct detector IDs have the incorrect position.
Setting `RewriteSpectraMap` to `True` will rewrite the detector ID per spectrum map by resetting the detectorID associated
with each spectrum on the output workspace to single available detectors on the instrument, and moving the repeated detector positions
in the appended workspace to available single detectors. This is only compatible with 2D Workspaces with common instruments and sufficient
available detectors. Also, monitor spectrum are skipped.



.. seealso:: :ref:`algm-ConjoinWorkspaces` for joining parts of the same workspace.

Usage
-----

**Example: Appending two workspaces**

.. testcode:: ExAppendSpectra

    ws = CreateSampleWorkspace(BankPixelWidth=1)
    ws2 = CreateSampleWorkspace(BankPixelWidth=2)
    for wsLoop in [ws,ws2]:
        print("Workspace '{}' has {} spectra beforehand".format(wsLoop, wsLoop.getNumberHistograms()))
    wsOut = AppendSpectra(ws, ws2)
    print("Workspace '{}' has {} spectra after AppendSpectra".format(wsOut, wsOut.getNumberHistograms()))


Output:

.. testoutput:: ExAppendSpectra

    Workspace 'ws' has 2 spectra beforehand
    Workspace 'ws2' has 8 spectra beforehand
    Workspace 'wsOut' has 10 spectra after AppendSpectra

**Example: Appending two workspaces**

.. testcode:: ExAppendSpectra

    ws = CreateSampleWorkspace(BankPixelWidth=1)
    ws2 = CreateSampleWorkspace(BankPixelWidth=1)
    for wsLoop in [ws,ws2]:
        print("Workspace '{}' has {} spectra beforehand".format(wsLoop, wsLoop.getNumberHistograms()))
    wsOut = AppendSpectra(ws, ws2, Number=4)
    print("Workspace '{}' has {} spectra after AppendSpectra".format(wsOut, wsOut.getNumberHistograms()))


Output:

.. testoutput:: ExAppendSpectra

    Workspace 'ws' has 2 spectra beforehand
    Workspace 'ws2' has 2 spectra beforehand
    Workspace 'wsOut' has 10 spectra after AppendSpectra

.. categories::

.. sourcelink::
