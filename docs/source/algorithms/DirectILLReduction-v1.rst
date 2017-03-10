.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This is the main data reduction algorithm in the ILL's time-of-flight reduction suite. It performs the last steps of the reduction workflow, namely vanadium normalisation and transformation to :math:`S(q,\omega)` space (optionally :math:`S(2\theta,\omega)`). The algorithm's workflow diagram is shown below:

.. diagram:: DirectILLReduction-v1_wkflw.dot

Input workspaces
################

*InputWorkspace* should contain data treated by :ref:`DirectILLCollectData <algm-DirectILLCollectData>` and, optionally, by :ref:`DirectILLApplySelfShielding <algm-DirectILLApplySelfShielding>`.

The mandatory *IntegratedVanadiumWorkspace* should have gone through :ref:`DirectILLIntegrateVanadium <algm-DirectILLIntegrateVanadium>`. This workspace is used for the vanadium normalisation.

*DiagnosticsWorkspace* should be a product of :ref:`DirectILLDiagnostics <algm-DirectILLDiagnostics>`. It is used to mask the spectra of *InputWorkspace*.

Outputs
#######

The algorithm will transform the time-of-flight and spectrum numbers of *InputWorkspace* into :math:`S(q,\omega)` at its output. For :math:`2\theta` to :math:`q` transformation, :ref:`SofQWNormalisedPolygon <algm-SofQWNormalisedPolygon>` is used. By default, the output is converted from histograms to point data, and :ref:`Transpose <algm-Transpose>` is applied to it. This behavior can be turned off by the *Transpose* property.

The optional :math:`S(2\theta,\omega)` output can be enabled by the *OutputSofThetaEnergyWorkspace*.

(Re)binning in energy and momentum transfer
###########################################

After conversion from time-of-flight to energy transfer, the binning may differ from spectrum to spectrum if the sample to detector distances are unequal. The :ref:`SofQWNormalisedPolygon <algm-SofQWNormalisedPolygon>` algorithm cannot work with such ragged workspaces and thus rebinning is necessary. The rebinning can be specified by the *EnergyRebinningParams* property. This is directly passed to :ref:`Rebin <algm-Rebin>` as the *Params* property. If *EnergyRebinningParams* is not specified, an automatic rebinning scheme is used:
- Find the spectrum with smallest bin border. Copy binning from this spectrum for negative energy transfers.
- For positive energy transfers, use the median bin width at zero energy transfer.

*QBinningParams* are passed to :ref:`SofQWNormalisedPolygon <algm-SofQWNormalisedPolygon>` and have the same format as *EnergyRebinningParamas*. If the property is not specified, :math:`q` is binned to ten times the median :math:`2\theta` steps between the spectra.

Usage
-----

**Example - Not implemented**

.. categories::

.. sourcelink::
