.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This is the main data reduction algorithm in :ref:`ILL's time-of-flight reduction suite <DirectILL>`. It performs the last steps of the reduction workflow, namely vanadium normalisation and transformation to :math:`S(q,\omega)` space (optionally :math:`S(2\theta,\omega)`). The algorithm's workflow diagram is shown below:

.. diagram:: DirectILLReduction-v1_wkflw.dot

Input workspaces
################

*InputWorkspace* should contain data treated by :ref:`DirectILLCollectData <algm-DirectILLCollectData>` and, optionally, by :ref:`DirectILLApplySelfShielding <algm-DirectILLApplySelfShielding>`.

The mandatory *IntegratedVanadiumWorkspace* should have gone through :ref:`DirectILLIntegrateVanadium <algm-DirectILLIntegrateVanadium>`. This workspace is used for the vanadium normalisation.

*DiagnosticsWorkspace* should be a product of :ref:`DirectILLDiagnostics <algm-DirectILLDiagnostics>`. It is used to mask the spectra of *InputWorkspace*.

Outputs
#######

The algorithm will transform the time-of-flight and spectrum numbers of *InputWorkspace* into :math:`S(q,\omega)` at its output. For :math:`2\theta` to :math:`q` transformation, :ref:`SofQWNormalisedPolygon <algm-SofQWNormalisedPolygon>` is used. By default, the output is transposed by :ref:`Transpose <algm-Transpose>`. This behavior can be turned off by the *Transpose* property.

The optional :math:`S(2\theta,\omega)` output can be enabled by the *OutputSofThetaEnergyWorkspace*.

Normalisation to absolute units
###############################

Normalisation to absolute units can be enabled by setting *AbsoluteUnitsNormalisation* to :literal:`'Absolute Units ON'`. In this case the data is multiplied by a factor

    :math:`f = \frac{N_V \sigma_V}{N_S}`

after normalisation to vanadium giving units of barn to the data. In the above, :math:`N_V` stands for the vanadium number density, :math:`\sigma_V` for vanadium total scattering cross section and :math:`N_S` sample number density. 

The material properties should be set for *InputWorkspace* and *IntegratedVanadiumWorkspace* by :ref:`SetSample <algm-SetSample>` before running this algorithm .

(Re)binning in energy and momentum transfer
###########################################

After conversion from time-of-flight to energy transfer, the binning may differ from spectrum to spectrum if the sample to detector distances are unequal. The :ref:`SofQWNormalisedPolygon <algm-SofQWNormalisedPolygon>` algorithm cannot work with such ragged workspaces and thus rebinning is necessary. The rebinning can be specified by the *EnergyRebinningParams* property. This is directly passed to :ref:`Rebin <algm-Rebin>` as the *Params* property. If *EnergyRebinningParams* is not specified, an automatic rebinning scheme is used:
- Find the spectrum with smallest bin border. Copy binning from this spectrum for negative energy transfers.
- For positive energy transfers, use the median bin width at zero energy transfer.

*QBinningParams* are passed to :ref:`SofQWNormalisedPolygon <algm-SofQWNormalisedPolygon>` and have the same format as *EnergyRebinningParamas*. If the property is not specified, :math:`q` is binned to a value that depends on the :math:`2\theta` separation of the detectors and the wavelength.

Transposing output
##################

After conversion to momentum transfer, the vertical axis of the data is in units of momentum transfer while the horizontal axis is in energy transfer. By default, the data is transposed such that momentum transfer is on the horizontal axis and energy transfer in the vertical. This can be turned off by setting *Transposing* to :literal:`'Transposing OFF'`.

Usage
-----

For usage of this algorithm, check the examples :ref:`here <DirectILL>`.

.. categories::

.. sourcelink::
