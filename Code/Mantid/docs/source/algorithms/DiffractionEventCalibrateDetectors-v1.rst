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
    import textwrap
    ws = CreateSampleWorkspace("Event",NumBanks=1,BankPixelWidth=1)
    ws = MoveInstrumentComponent(Workspace='ws', ComponentName='bank1', X=0.5, RelativePosition=False)
    DiffractionEventCalibrateDetectors(InputWorkspace='ws', Params = "1.9, 0.001, 2.2", 
        LocationOfPeakToOptimize=2.038, BankName="bank1", DetCalFilename="Test.DetCal")
    with open(os.path.join(config["defaultsave.directory"], 'Test.DetCal'), 'r') as fin:
        print textwrap.fill(fin.read(), 70)
    os.remove(os.path.join(config["defaultsave.directory"], 'Test.DetCal'))

Output:

.. testoutput:: ExDiffractionEventCalibrateDetectors

    5  1  1  1  0.8  0.8  0.2000  50.098  50.0979 -0.0384774 0.0479424
    0.706984 0.0179703 0.707001  0.706995 0.0179703 0.70699


.. categories::

