.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Moves the detectors in an instrument to optimize the maximum intensity
of each detector using gsl\_multimin\_fminimizer\_nmsimplex. Only bin
data close to peak you wish to maximize.

Usage
-----

**Example: move detectors to maximize intensity**

.. testcode:: ExDiffractionEventCalibrateDetectors
                   
    import os
    ws = CreateSampleWorkspace("Event",NumBanks=1,BankPixelWidth=1)
    ws = MoveInstrumentComponent(Workspace='ws', ComponentName='bank1', X=0.5, RelativePosition=False)
    wsD = ConvertUnits(InputWorkspace='ws',  Target='dSpacing')
    maxD = Max(wsD)
    #Peak is at 2.69 in dSpace
    DiffractionEventCalibrateDetectors(InputWorkspace='ws', Params = "2.6, 0.001, 2.8",
        LocationOfPeakToOptimize=2.67, BankName="bank1", DetCalFilename="Test.DetCal")
    LoadIsawDetCal(InputWorkspace='ws',Filename=os.path.join(config["defaultsave.directory"], 'Test.DetCal'))
    ws = ConvertUnits(InputWorkspace='ws',  Target='dSpacing')
    maxA = Max(ws)
    print "Peak in dSpace", 0.5*(maxD.readX(0)[0]+maxD.readX(0)[1])
    print "Peak from calibration", 0.5*(maxA.readX(0)[0]+maxA.readX(0)[1])

Output:

.. testoutput:: ExDiffractionEventCalibrateDetectors

    Peak in dSpace 2.69077317913
    Peak from calibration 2.67124691143


.. categories::

