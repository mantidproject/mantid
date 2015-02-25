.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This is the top-level workflow algorithm for direct geometry
spectrometer data reduction. This algorithm is responsible for gathering
the necessary parameters and generating calls to other workflow or
standard algorithms.

Workflow
########

Parameters for the child algorithms are not shown due to the sheer number.
They will be detailed in the child algorithm diagrams. Items in
parallelograms are output workspaces from their respective algorithms.
Not all output workspaces are subsequently used by other algorithms.

.. diagram:: DgsReduction-v1_wkflw.dot

Usage
-----

**Example - Simplest SNS Script**

.. testcode:: SimpleSnsEx

    config['default.facility'] = "SNS"
    # SNS reduction does not require IncidentEnergyGuess since the NeXus
    # files contain an EnergyRequest log.
    ws = DgsReduction(SampleInputFile="CNCS_7860_event.nxs")
    # More than one output from DgsReduction
    # (E vs Phi Workspace, Name of Property Manager)
    w = ws[0]
    # Check the conversion
    xaxis = w.getAxis(0).getUnit()
    print "X axis =", xaxis.caption(), "/", xaxis.symbol()
    print "Energy mode =", w.getEMode()
    print "Workspace type =", w.id()

Output:

.. testoutput:: SimpleSnsEx

    X axis = Energy transfer / meV
    Energy mode = Direct
    Workspace type = Workspace2D

**Example - Simplest ISIS Script**

.. testcode:: SimpleIsisEx

    config['default.facility'] = "ISIS"
    # ISIS reduction required IncidentEnergyGuess since RAW files don't
    # have a corresponding log.
    ws = DgsReduction(SampleInputFile="MAR11001.raw",
                      IncidentEnergyGuess=12)
    # More than one output from DgsRedution
    # (E vs Phi Workspace, Name of Property Manager)
    w = ws[0]
    # Check the conversion
    xaxis = w.getAxis(0).getUnit()
    print "X axis =", xaxis.caption(), "/", xaxis.symbol()
    print "Energy mode =", w.getEMode()
    print "Workspace type =", w.id()

Output:

.. testoutput:: SimpleIsisEx

    X axis = Energy transfer / meV
    Energy mode = Direct
    Workspace type = Workspace2D

**Example - Get as Eventworkspace for SNS**

The following shows that event data can be the result of the redcuction instead of
histogram data.

.. testcode:: EventSnsEx

    config['default.facility'] = "SNS"
    ws = DgsReduction(SampleInputFile="CNCS_7860_event.nxs",
                      IncidentBeamNormalisation="ByCurrent",
                      SofPhiEIsDistribution=False)
    w = ws[0]
    print "Workspace type =", w.id()
    print "Number of events =", w.getNumberEvents()

Output:

.. testoutput:: EventSnsEx

    Workspace type = EventWorkspace
    Number of events = 78037

**Example - Correct Data Before Reduction for SNS**

The following is a contrived example to show that the data can be loaded and
corrected and/or filtered before passing it to the algorithm. The key here is that
the \*InputWorkspace properties must be used. It also shows a couple of more
properties being used.

.. testcode:: CorrectSnsEx

    config['default.facility'] = "SNS"
    ws = Load("CNCS_7860_event.nxs", LoadMonitors=True)
    monitor = ws[1]
    valC3 = ws[0].getRun()['Phase3'].getStatistics().median
    ws = FilterByLogValue(ws[0], LogName="Phase3", MinimumValue=valC3-0.3,
                          MaximumValue=valC3+0.3)
    # Although CNCS doesn't use its monitors, this is how instruments that do need
    # to call the algorithm.
    ws = DgsReduction(SampleInputWorkspace=ws, SampleInputMonitorWorkspace=monitor,
                      IncidentBeamNormalisation="ByCurrent", SofPhiEIsDistribution=False)
    w = ws[0]
    print "Workspace type =", w.id()
    print "Number of events =", w.getNumberEvents()

Output:

.. testoutput:: CorrectSnsEx

    Workspace type = EventWorkspace
    Number of events = 2178

.. categories::
