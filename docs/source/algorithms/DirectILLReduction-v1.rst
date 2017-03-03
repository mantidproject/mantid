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

Binning in both energy and momentun transfer is automatic by default. For energy rebinning, the entire energy transfer range is rebinned to the bin width at the elastic bin (zero energy transfer). :math:`q`, in turn, is binned to median :math:`2\theta` steps between the spectra. There is an option to rebin energy to the median bin width as well. This can be changed by *EnergyRebinningMode*.

The automatic energy rebinning might in some cases result in an enormous number of bins. The number of bins is thus limited to ten times the number of bins before rebinning. This limit can be controlled by *EnergyBinCountLimit*.

If it is desirable to give manual binning parameters, both *EnergyRebinningMode* and *QBinningMode* can be set to manual mode. In this case, *EnergyRebinningParams* and *QBinningParams* have to be given as well. For the format of these properties, see :ref:`Rebin <algm-Rebin>`.

Usage
-----

**Example - Not implemented**

.. categories::

.. sourcelink::
