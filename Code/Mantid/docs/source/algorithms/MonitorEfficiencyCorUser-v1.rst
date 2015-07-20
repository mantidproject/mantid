.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Normalizes the counts by monitor counts with additional efficiency correction.

Corrects the monitor efficiency according to the formula contained in TOFTOF's Parameters file.

The values of the monitor counts and incident energy are stored in the SampleLogs of the input workspace.

To date this algorithm only supports the TOFTOF instrument.


Restrictions
###################################

A formula named "formula\_eff" must be defined in the instrument
parameters file.

Usage
-----

**Example**

.. testcode:: ExMonitorEfficiencyCorUser

    import numpy as np
    #Load data
   
    wsSample = LoadMLZ(Filename='TOFTOFTestdata.nxs')

    wsNorm = MonitorEfficiencyCorUser(wsSample)
    # Check calculation of normalization coefficient
    wsCheck = Divide(wsSample,wsNorm)
    print 'Coefficient of proportionality between Input and Output of MonitorEfficiencyCorUser algorithm:'
    print wsCheck.readY(102)[1]
    monitor_counts = float(mtd['wsSample'].getRun().getLogData('monitor_counts').value)
    Ei = float(mtd['wsSample'].getRun().getLogData('Ei').value)
    print 'Coefficient from theoretical formula = monitor_counts * sqrt(Ei/25.3) ', monitor_counts*np.sqrt(Ei/25.3)
  

Output:

.. testoutput:: ExMonitorEfficiencyCorUSer

    Coefficient of proportionality between Input and Output of MonitorEfficiencyCorUser algorithm:
    41038.4323645
    Coefficient from theoretical formula = monitor_counts * sqrt(Ei/25.3)  41038.4323645

.. categories::

.. sourcelink::
