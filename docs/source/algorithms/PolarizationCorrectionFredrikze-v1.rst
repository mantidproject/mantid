
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Performs wavelength polarization correction on a TOF reflectometer spectrometer.

The algorithm is based on the paper Fredrikze, H, et al. "Calibration of a polarized neutron reflectometer" Physica B 297 (2001) [#FREDRIKZE]_.

Polarizer and Analyzer efficiencies are calculated and used to perform an intensity correction on the input workspace. The input workspace(s) are in units of wavelength (inverse angstroms).

In the ideal case :math:`P_{p} = P_{a} = A_{p} = A_{a} = 1`.

:math:`\rho = \frac{P_{a}}{P_{p}}` where rho is bounded by, but inclusive of 0 and 1. Since this ratio is wavelength dependent, rho is a polynomial, which is expressed as a function of wavelength. For example:
:math:`\rho(\lambda) =\sum\limits_{i=0}^{i=2} K_{i}\centerdot\lambda^i`, can be provided as :math:`K_{0}, K_{1}, K_{2}`.

:math:`\alpha = \frac{A_{a}}{A_{p}}` where alpha is bounded by, but inclusive of 0 and 1. Since this ratio is wavelength dependent, alpha is a polynomial, which is expressed as a function of wavelength. For example:
:math:`\alpha(\lambda) =\sum\limits_{i=0}^{i=2} K_{i}\centerdot\lambda^i`, can be provided as :math:`K_{0}, K_{1}, K_{2}`.

Spin State Configurations
#########################
The algorithm expresses the order of the workspaces in the input and output groups in terms of spin states. These orders are specified using two properties:

**Input Spin State:**
This property determines the order of the spin states in the input workspace group

- :literal:`'pp, pa, ap, aa'`
   Full polarization analysis (:literal:`'PA'`): both parallel, parallel-anti-parallel, anti-parallel-parallel, both  anti-parallel. Four input workspaces are required. The spin states can be provided in any order and should match the order of the workspaces in the input group. The default order is :literal:`pp, pa, ap, aa`.

- :literal:`'p, a'`
   Polarized Neutron Reflectivity (:literal:`'PNR'`):  parallel, anti-parallel. Two input workspaces are required. The spin states can be provided in any order and should match the order of the workspaces in the input group. The default order is :literal:`p, a`.

**Output Spin State:**
This property determines the order of the spin states in the output workspace group. Similar to the input configuration, users can specify output spin states in any order.

If the ``AddSpinStateToLog`` property has been set to ``True`` then a sample log entry called ``spin_state_ORSO`` is added to each output child workspace.
This log entry specifies the spin state of the data using the notation from the Reflectometry ORSO data standard [#ORSO]_.

Output from Full Polarization Analysis
--------------------------------------

The output of this algorithm is a :ref:WorkspaceGroup. The resulting :ref:WorkspaceGroup  will have 4 entries, corresponding to the specified output spin state orders. If no specific output spin states are specified, it will be formatted as follows:

==============  ================
Entry in group  Measurement
==============  ================
1               :math:`I_{pp}`
2               :math:`I_{pa}`
3               :math:`I_{ap}`
4               :math:`I_{aa}`
==============  ================

Output from Polarized Neutron Reflectivity
------------------------------------------

The output of this algorithm is a :ref:`WorkspaceGroup <WorkspaceGroup>`. The resulting :ref:`WorkspaceGroup <WorkspaceGroup>` will have 2 entries, corresponding to the specified output spin state orders. If no specific output spin states are specified, it will be formatted as follows:

==============  ================
Entry in group  Measurement
==============  ================
1               :math:`I_{p}`
2               :math:`I_{a}`
==============  ================

Usage
-----

.. include:: ../usagedata-note.txt

**Example - PolarizationCorrectionFredrikze (PNR)**

.. testcode:: PolarizationCorrectionFredrikze (PNR)

    # import mantid algorithms, numpy and matplotlib
    from mantid.simpleapi import *
    import matplotlib.pyplot as plt
    import numpy as np

    run = Load("POLREF00004699.nxs")
    ws1 = CreateWorkspace([0.01, 17.0, 34.0], [50000, 50000])
    eff = JoinISISPolarizationEfficiencies(Pp=ws1,   Rho=ws1, Ap=ws1, Alpha=ws1)
    run = ConvertUnits(run, "Wavelength")
    eff = ConvertUnits(eff, "Wavelength")
    eff = RebinToWorkspace(eff, run.getItem(0), True)

    out = PolarizationCorrectionFredrikze(run, eff, "PNR", InputSpinStates="p, a", OutputSpinStates="p, a", AddSpinStateToLog=True)

    for i, orig in enumerate(run):
    	corrected = out[i]
    	y_idx = run[0].yIndexOfX(15.0)
    	ratio = corrected.readY(29)[y_idx] / orig.readY(29)[y_idx]
    	print(f"Ratio of corrected {corrected.name()} and original {orig.name()} intensity at 15A: {round(ratio, 4)}")

Output:

.. testoutput:: PolarizationCorrectionFredrikze (PNR)

   Ratio of corrected out_1 and original run_1 intensity at 15A: 1.0005
   Ratio of corrected out_2 and original run_2 intensity at 15A: 0.9508

**Example - PolarizationCorrectionFredrikze (PA)**

.. testcode:: PolarizationCorrectionFredrikze (PA)

    from mantid.simpleapi import *
    import matplotlib.pyplot as plt
    import numpy as np

    run1 = Load("POLREF00004699.nxs")
    run2 = run1 * 1.2
    run = GroupWorkspaces([run1, run2])
    ws1 = CreateWorkspace([0.01, 17.0, 34.0], [50000, 50000])
    eff = JoinISISPolarizationEfficiencies(Pp=ws1,   Rho=ws1, Ap=ws1, Alpha=ws1)
    run = ConvertUnits(run, "Wavelength")
    eff = ConvertUnits(eff, "Wavelength")
    eff = RebinToWorkspace(eff, run.getItem(0), True)

    out = PolarizationCorrectionFredrikze(run, eff, "PA", InputSpinStates="pp, pa, ap, aa", OutputSpinStates="pp, pa, ap, aa", AddSpinStateToLog=True)

    for i, orig in enumerate(run):
    	corrected = out[i]
    	y_idx = run[0].yIndexOfX(15.0)
    	ratio = corrected.readY(29)[y_idx] / orig.readY(29)[y_idx]
    	print(f"Ratio of corrected {corrected.name()} and original {orig.name()} intensity at 15A: {round(ratio, 4)}")

.. testoutput:: PolarizationCorrectionFredrikze (PA)

   Ratio of corrected out_1 and original run1_1 intensity at 15A: 1.0025
   Ratio of corrected out_2 and original run1_2 intensity at 15A: 0.9527
   Ratio of corrected out_3 and original run2_1 intensity at 15A: 0.8355
   Ratio of corrected out_4 and original run2_2 intensity at 15A: 0.794

References
----------

.. [#FREDRIKZE] Fredrikze, H, et al., *Calibration of a polarized neutron reflectometer*, Physica B 297 (2001)
             `doi: 10.1016/S0921-4526(00)00823-1 <https://doi.org/10.1016/S0921-4526(00)00823-1>`_
.. [#ORSO] ORSO file format specification: `https://www.reflectometry.org/file_format/specification <https://www.reflectometry.org/file_format/specification>`_

.. categories::

.. sourcelink::
