.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm normalises the neutron counts by monitor counts with an additional efficiency correction.

To date this algorithm only supports the TOFTOF instrument.

The monitor counts is the total count and it is stored in the SampleLogs of the input workspace.

This count is corrected taking into account the monitor efficiency. The formula used for the correction is stored in the Parameters file and requires the incident energy (Ei), which is stored in the SampleLogs of the input workspace.

The corrected value of the monitor counts is used to normalise the input workspace.


Restrictions
###################################

A formula named "formula\_eff" must be defined in the instrument
parameters file. It is defined as "monitor counts * sqrt(Ei/25.3)"

The incident energy Ei and the monitor counts are read in the SampleLogs of the input workspace.

Usage
-----

**Example**

.. testcode:: ExMonitorEfficiencyCorUser

    import numpy as np
    wsSample = LoadMLZ(Filename='TOFTOFTestdata.nxs')
    wsNorm = MonitorEfficiencyCorUser(wsSample )
    # Input and output workspaces have the same structure
    print 'Number of histograms of the input and output workspaces:'
    print wsSample.getNumberHistograms(), wsNorm.getNumberHistograms()
    print 'Number of time channels of the input an output workspaces:'
    print wsSample.blocksize(), wsNorm.blocksize()
    # Check calculation of normalisation coefficient between input and output workspaces
    wsCheck = Divide(wsSample,wsNorm)
    print "Coefficient of proportionality between Input and Output of MonitorEfficiencyCorUser algorithm: %5.3f" % wsCheck.readY(102)[1]
    # Read the values of the incident energy and of the monitor counts from the SampleLogs of wsSample
    monitor_counts = float(mtd['wsSample'].getRun().getLogData('monitor_counts').value)
    Ei = float(mtd['wsSample'].getRun().getLogData('Ei').value)
    print "Coefficient from theoretical formula = monitor_counts * sqrt(Ei/25.3): %5.3f" % (monitor_counts*np.sqrt(Ei/25.3))
 
 
Output:

.. testoutput:: ExMonitorEfficiencyCorUser

    Number of histograms of the input and output workspaces:
    1006 1006
    Number of time channels of the input an output workspaces:
    1024 1024
    Coefficient of proportionality between Input and Output of MonitorEfficiencyCorUser algorithm: 41038.432
    Coefficient from theoretical formula = monitor_counts * sqrt(Ei/25.3): 41038.432 

.. categories::

.. sourcelink::
