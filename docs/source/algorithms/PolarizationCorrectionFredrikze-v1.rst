
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

**Input Spin State Order:**
This property determines the order of the spin states in the input workspace group

- :literal:`'pp, pa, ap, aa'`
   Full polarization analysis (:literal:`'PA'`): both parallel, parallel-anti-parallel, anti-parallel-parallel, both  anti-parallel. Four input workspaces are required. The spin states can be provided in any order and should match the order of the workspaces in the input group.

- :literal:`'p, a'`
   Polarized Neutron Reflectivity (:literal:`'PNR'`):  parallel, anti-parallel. Two input workspaces are required. The spin states can be provided in any order and should match the order of the workspaces in the input group.

.. note::
    the default order used by the algorithm if none is specified are :literal:`pp, pa, ap, aa` for full polarization analysis or :literal:`p, a` for Polarized Neutron Reflectivity.

**Output Spin State Order:**
This property determines the order of the spin states in the output workspace group. Similar to the input configuration, users can specify output spin states in any order.

Output from Full Polarization Analysis
--------------------------------------

The output of this algorithm is a :ref:`WorkspaceGroup <WorkspaceGroup>`, the resulting :ref:`WorkspaceGroup <WorkspaceGroup>` will have 4 entries, corresponding to the output specified spin state orders, if no specific output spin states are specified, it will be formatted as follows:

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
    ws1 = CreateWorkspace([1, 2, 3], [1, 1])
    eff = JoinISISPolarizationEfficiencies(Pp=ws1,   Rho=ws1, Ap=ws1, Alpha=ws1)
    run = ConvertUnits(run, "Wavelength")
    eff = ConvertUnits(eff, "Wavelength")
    eff = RebinToWorkspace(eff, run.getItem(0), True)
    out = PolarizationCorrectionFredrikze(run, eff, "PNR", InputSpinStates="p, a", OutputSpinStates="p, a")

    orig = mtd['POLREF00004699_1']
    corr = mtd['out_1']
    index = orig.yIndexOfX(10000.)
    ratio_1 = corr.readY(0)[index] / orig.readY(0)[index]
    print("Ratio of corrected 'corrected_1' and original 'input_data_1' intensity at 15A: {:.4}".format(ratio_1))
    orig = mtd['POLREF00004699_2']
    corr = mtd['out_2']
    index = orig.yIndexOfX(10000.)
    ratio_2 = corr.readY(0)[index] / orig.readY(0)[index]
    print("Ratio of corrected 'corrected_2' and original 'input_data_2' intensity at 15A: {:.4}".format(ratio_2))

Output:

.. testoutput:: PolarizationCorrectionFredrikze (PNR)

   Ratio of corrected 'out_1' and original 'input_data_1' intensity at 10000A: 0.9996
   Ratio of corrected 'out_2' and original 'input_data_2' intensity at 10000A: 1.0010

**Example - PolarizationCorrectionFredrikze (PA)**

.. testcode:: PolarizationCorrectionFredrikze (PA)

    from mantid.simpleapi import *
    import matplotlib.pyplot as plt
    import numpy as np

    run1 = Load("POLREF00004699.nxs")
    run2 = run1 * 1.2
    run = GroupWorkspaces([run1, run2])
    ws1 = CreateWorkspace([1, 2, 3], [1, 1])
    eff = JoinISISPolarizationEfficiencies(Pp=ws1,   Rho=ws1, Ap=ws1, Alpha=ws1)
    run = ConvertUnits(run, "Wavelength")
    eff = ConvertUnits(eff, "Wavelength")

    eff = RebinToWorkspace(eff, run.getItem(0), True)

    out = PolarizationCorrectionFredrikze(run, eff, "PA", InputSpinStates="pp, pa, ap, aa", OutputSpinStates="pp, pa, ap, aa")
    orig = mtd['POLREF00004699_1']
    corr = mtd['out_1']
    index = orig.yIndexOfX(15.)
    ratio_1 = corr.readY(0)[index] / orig.readY(0)[index]
    print("Ratio of corrected 'out_1' and original 'input_data_1' intensity at 15A: {:.4}".format(ratio_1))
    orig = mtd['POLREF00004699_2']
    corr = mtd['out_2']
    index = orig.yIndexOfX(15.)
    ratio_2 = corr.readY(0)[index] / orig.readY(0)[index]
    print("Ratio of corrected 'out_2' and original 'input_data_2' intensity at 15A: {:.4}".format(ratio_2))
    orig = mtd['run2_1']
    corr = mtd['out_3']
    index = orig.yIndexOfX(15.)
    ratio_3 = corr.readY(0)[index] / orig.readY(0)[index]
    print("Ratio of corrected 'out_3' and original 'input_data_3' intensity at 15A: {:.4}".format(ratio_3))
    orig = mtd['run2_2']
    corr = mtd['out_4']
    index = orig.yIndexOfX(15.)
    ratio_4 = corr.readY(0)[index] / orig.readY(0)[index]
    print("Ratio of corrected 'out_4' and original 'input_data_4' intensity at 15A: {:.4}".format(ratio_4))

.. testoutput:: PolarizationCorrectionFredrikze (PA)

   Ratio of corrected 'out_1' and original 'input_data_1' intensity at 15A: 0.9337
   Ratio of corrected 'out_2' and original 'input_data_2' intensity at 15A: 0.9478
   Ratio of corrected 'out_3' and original 'input_data_3' intensity at 15A: 1.088
   Ratio of corrected 'out_4' and original 'input_data_4' intensity at 15A: 1.105

References
----------

.. [#FREDRIKZE] Fredrikze, H, et al., *Calibration of a polarized neutron reflectometer*, Physica B 297 (2001)
             `doi: 10.1016/S0921-4526(00)00823-1 <https://doi.org/10.1016/S0921-4526(00)00823-1>`

.. categories::

.. sourcelink::