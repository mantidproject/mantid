To update your documentation based on the new approach of using spin states directly rather than flipper configurations, you need to revise the relevant sections to reflect this change. Here’s an updated version of your documentation with a focus on the new spin state configurations and removing references to flipper configurations:

---

.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Performs wavelength polarization correction on a TOF reflectometer spectrometer.

The algorithm is based on the paper Fredrikze, H, et al. "Calibration of a polarized neutron reflectometer" Physica B 297 (2001).

Polarizer and Analyzer efficiencies are calculated and used to perform an intensity correction on the input workspace. The input workspace(s) are in units of wavelength (inverse angstroms).

In the ideal case :math:`P_{p} = P_{a} = A_{p} = A_{a} = 1`.

:math:`\rho = \frac{P_{a}}{P_{p}}` where rho is bounded by, but inclusive of 0 and 1. Since this ratio is wavelength dependent, rho is a polynomial, which is expressed as a function of wavelength. For example:
:math:`\rho(\lambda) =\sum\limits_{i=0}^{i=2} K_{i}\centerdot\lambda^i`, can be provided as :math:`K_{0}, K_{1}, K_{2}`.

:math:`\alpha = \frac{A_{a}}{A_{p}}` where alpha is bounded by, but inclusive of 0 and 1. Since this ratio is wavelength dependent, alpha is a polynomial, which is expressed as a function of wavelength. For example:
:math:`\alpha(\lambda) =\sum\limits_{i=0}^{i=2} K_{i}\centerdot\lambda^i`, can be provided as :math:`K_{0}, K_{1}, K_{2}`.

Spin State Configurations
#########################
The algorithm directly uses spin states to define the measurement setup. The spin states are specified using two properties:

**Input Spin State Order:**
This property determines the order of the spin states in the input workspace group

- :literal:`'pp, pa, ap, aa'`
   Full polarization corrections: both parallel, parallel-anti-parallel, anti-parallel-parallel, both  anti-parallel. Four input workspaces are required. The spin states can be provided in any order and should match the order of the workspaces in the input group.

- :literal:`'p, a'`
   PNR polarization corrections:  parallel, anti-parallel. Two input workspaces are required. The spin states can be provided in any order and should match the order of the workspaces in the input group.

.. note::
    the default order used by the algorithm if none is specified are :literal:`pp, pa, ap, aa` for full polarization analysis or :literal:`p, a` for PNR.

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

This format allows for detailed analysis of the polarization states.

.. categories::

.. sourcelink::

---

