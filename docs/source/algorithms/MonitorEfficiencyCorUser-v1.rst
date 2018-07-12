.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm normalises the neutron counts by monitor counts with an additional efficiency correction.

The monitor counts is the total count and it is stored in the SampleLogs of the input workspace.

This count is corrected taking into account the monitor efficiency. The formula used for the correction is stored in the Parameters file and requires the incident energy (:math:`E_i`), which is stored in the SampleLogs of the input workspace.

The corrected value of the monitor counts is used to normalise the input workspace.


Restrictions
###################################

- A formula named "formula\_eff" must be defined in the instrument parameters file. For TOFTOF and DNS instruments it is defined as :math:`M\cdot\sqrt{\frac{E_i}{25.3}}`. The incident energy :math:`E_i` and the monitor counts :math:`M` are read in the sample logs of the input workspace.
- Either sample log "monitor\_counts" must be present in the InputWorkspace or the name of the sample log containing monitor counts must be defined under "monitor\_counts\_log" parameter in the instrument parameters file.
- Input workspace must have "Ei" sample log.


Usage
-----

**Example**

.. testcode:: ExMonitorEfficiencyCorUser

    import numpy as np
    wsSample = LoadMLZ(Filename='TOFTOFTestdata.nxs')
    wsNorm = MonitorEfficiencyCorUser(wsSample )
    # Input and output workspaces have the same structure
    print('Number of histograms of the input and output workspaces:')
    print('{} {}'.format(wsSample.getNumberHistograms(), wsNorm.getNumberHistograms()))
    print('Number of time channels of the input an output workspaces:')
    print('{} {}'.format( wsSample.blocksize(), wsNorm.blocksize()))
    # Check calculation of normalisation coefficient between input and output workspaces
    wsCheck = Divide(wsSample,wsNorm)
    print("Coefficient of proportionality between Input and Output of MonitorEfficiencyCorUser algorithm: {:.3f}".format(wsCheck.readY(102)[1]))
    # Read the values of the incident energy and of the monitor counts from the SampleLogs of wsSample
    monitor_counts = float(mtd['wsSample'].getRun().getLogData('monitor_counts').value)
    Ei = float(mtd['wsSample'].getRun().getLogData('Ei').value)
    print("Coefficient from theoretical formula = monitor_counts * sqrt(Ei/25.3): {:.3f}".format(monitor_counts*np.sqrt(Ei/25.3)))
 
 
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
