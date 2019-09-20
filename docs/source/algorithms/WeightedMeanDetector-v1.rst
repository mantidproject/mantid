.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This Algorithm produces a weighted mean of detectors in a workspace.
`WeightedMeanDetector` is designed to accept two workspaces obtained from
the ATLAS suite: the first is DCS containing the corrected total scattering
:math:`I(Q)` for each detector group, the second is a SLF containing the
self scattering for each detector group.

The Algorithm also accepts three files describing the weighting to be used
by `WeightedMeanDetector`.
- .alf file: Containing alpha values :math:`\alpha_i` for each detector group:

+---+------+
| 8 |      |
+---+------+
| 1 | 0.80 |
+---+------+
| 2 | 0.90 |
+---+------+
| 3 | 0.86 |
+---+------+
| 4 | 0.95 |
+---+------+
| 5 | 1.01 |
+---+------+
| 6 | 1.05 |
+---+------+
| 7 | 1.16 |
+---+------+
| 8 | 1.20 |
+---+------+

- .lim file: Containing the upper :math:`Qmax_i` and lower :math:`Qmin_i` boundaries for each detector group,
  the values in the second column indicate if the detector should be used:

+---+---+-----+------+
| 8 |   |     |      |
+---+---+-----+------+
| 1 | 0 |     |      |
+---+---+-----+------+
| 2 | 0 |     |      |
+---+---+-----+------+
| 3 | 1 | 0.9 | 7.3  |
+---+---+-----+------+
| 4 | 1 | 2.3 | 9.8  |
+---+---+-----+------+
| 5 | 1 | 6.2 | 13.2 |
+---+---+-----+------+
| 6 | 1 | 10  | 14   |
+---+---+-----+------+
| 7 | 1 | 4   | 14   |
+---+---+-----+------+
| 8 | 1 | 10  | 15   |
+---+---+-----+------+

- .lin file: Containing a gradient :math:`A_i` and intercept :math:`B_i` describing a linear correction,
  the values in the second column indicate if the detector has a linear correction:

+---+---+------+-------+
| 8 |   |      |       |
+---+---+------+-------+
| 1 | 0 |      |       |
+---+---+------+-------+
| 2 | 0 |      |       |
+---+---+------+-------+
| 3 | 0 |      |       |
+---+---+------+-------+
| 4 | 1 | 0    | 0.045 |
+---+---+------+-------+
| 5 | 1 | 0    | 0.047 |
+---+---+------+-------+
| 6 | 1 | 0.1  | 0.044 |
+---+---+------+-------+
| 7 | 1 | 0.02 | 0.037 |
+---+---+------+-------+
| 8 | 1 | 0.05 | 0.036 |
+---+---+------+-------+

The corrected distinct scattering :math:`i_{i}(Q)` for each detector bank is calculated as:

.. math::

   i_{i}(Q) = \alpha_{i} I_{i} (Q) - I_{i}^{s} (Q) + A_{i} Q + B_{i}

The output workspace contains the mean of these scatterings for each value of :math:`Q`
between :math:`Qmax_i` and lower :math:`Qmin_i` for each detector.

Usage
-----

.. code:: Python

   from mantid.simpleapi import *

   Load(
       Filename = "gem61910.DCS",
       OutputWorkspace = "gem61910_DCS")
   ConvertToHistogram(
       InputWorkspace = "gem61910_DCS",
       OutputWorkspace = "gem61910_DCS")
   Load(
       Filename = "gem61910.SLF",
       OutputWorkspace = "gem61910_SLF")
   ConvertToHistogram(
       InputWorkspace = "gem61910_SLF",
       OutputWorkspace = "gem61910_SLF")
   WeightedMeanDetector(
       DCSWorkspace = gem61910_DCS,
       SLFWorkspace = gem61910_SLF,
       OutputWorkspace = gem61910_merged,
       AlfFile = gem61910.alf,
       LimFile = gem61910.lim,
       LinFile = gem61910.lin)

.. categories::

.. sourcelink::
