.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm removes the background present in PSI bin data, loaded using LoadPSIMuonBin

The background is removed from the original data through the following formula,

.. math:: y_{cor}(t) = y_{org}(t) - B,

where :math:`y_{cor}(t)` is the corrected bin data, :math:`y_{org}(t)` the original counts data and :math:`B` is the
flat background.

To obtain the flat background, :math:`B`, the second-half of the good raw-data is fitted with the following function,

.. math:: f(t) = \mbox{A}e^{-\lambda t} + B

where the first term represents a ExpDecay function, see :ref:`func-ExpDecayMuon`.
The good raw-data is defined as being from the bin containing the `first good data` to the bin containing the `last good data`.

The algorithm takes in an input workspace and performs the correction inplace on the workspace. The number of iterations
can be specified through an optional parameter. If a poor quality fit is returned, a warning will be displayed in the
Mantid logger.

Usage
-----

.. testcode:: CalculateBackgroundForTestData

    # import mantid algorithms, numpy and matplotlib
    from mantid.simpleapi import *
    import numpy as np
    # Generate shifted ExpDecay data
    A = 1200
    Background = 20
    Lambda = 0.5;
    time = np.linspace(0, 10, 100)
    func = lambda t: A*np.exp(-Lambda*t) + Background
    counts = np.array([func(ti) for ti in time])

    # Create workspaces
    input_workspace = CreateWorkspace(time, counts)
    input_workspace.setYUnit("Counts")
    run = input_workspace.getRun()
    run.addProperty("First good spectra 0",10,"None",True)
    run.addProperty("Last good spectra 0",99,"None",True)
    workspace_copy = input_workspace.clone()

    # Run PSIBackgroundSubtraction Algorithm
    PSIBackgroundSubtraction(input_workspace)

    # Find the difference between the workspaces
    workspace_diff = Minus(workspace_copy, input_workspace)
    diffs = np.round(workspace_diff.readY(0),4)
    # The counts in workspace diff should be a flat line corresponding to the background
    print("Differences in counts are: {}".format(diffs))

Output:

.. testoutput:: CalculateBackgroundForTestData

    Differences in counts are: [20. 20. 20. 20. 20. 20. 20. 20. 20. 20. 20. 20. 20. 20. 20. 20. 20. 20.
     20. 20. 20. 20. 20. 20. 20. 20. 20. 20. 20. 20. 20. 20. 20. 20. 20. 20.
     20. 20. 20. 20. 20. 20. 20. 20. 20. 20. 20. 20. 20. 20. 20. 20. 20. 20.
     20. 20. 20. 20. 20. 20. 20. 20. 20. 20. 20. 20. 20. 20. 20. 20. 20. 20.
     20. 20. 20. 20. 20. 20. 20. 20. 20. 20. 20. 20. 20. 20. 20. 20. 20. 20.
     20. 20. 20. 20. 20. 20. 20. 20. 20. 20.]

.. testcode:: CalculateBackgroundWithAdditionalFunction

    import numpy as np

    # Generate shifted ExpDecay data
    A = 1200
    Background = 20
    Lambda = 0.5
    time = np.linspace(0, 10, 500)
    func = lambda t: A*np.exp(-Lambda*t) + Background + 0.5*A*np.sin(50.0*t)
    counts = np.array([func(ti) for ti in time])

    # Create workspaces
    input_workspace = CreateWorkspace(time, counts)
    input_workspace.setYUnit("Counts")
    run = input_workspace.getRun()
    run.addProperty("First good spectra 0", 10, "None", True)
    run.addProperty("Last good spectra 0", 99, "None", True)
    workspace_copy = input_workspace.clone()

    # Run PSIBackgroundSubtraction Algorithm
    function = "name=GausOsc,A=500,Sigma=0.2,Frequency=40,Phi=0"
    PSIBackgroundSubtraction(input_workspace, StartX=5, EndX=10, Function=function)

    # Find the difference between the workspaces
    workspace_diff = Minus(workspace_copy, input_workspace)
    diffs = np.round(workspace_diff.readY(0), 4)
    # The counts in workspace diff should be a flat line corresponding to the background
    print("Difference in first count is: {}".format(diffs[0]))
    print("Difference in middle count is: {}".format(diffs[int(len(diffs)/2)]))
    print("Difference in last count is: {}".format(diffs[-1]))

Output:

.. testoutput:: CalculateBackgroundWithAdditionalFunction

    Difference in first count is: 20.0
    Difference in middle count is: 20.0
    Difference in last count is: 20.0

.. categories::

.. sourcelink::
    :h: Framework/Muon/inc/MantidMuon/PSIBackgroundSubtraction.h
    :cpp: Framework/Muon/src/PSIBackgroundSubtraction.cpp
