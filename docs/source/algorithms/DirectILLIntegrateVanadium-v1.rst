.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm integrates the workspace given in *InputWorkspace* using the :ref:`ComputeCalibrationCoefVan <algm-ComputeCalibrationCoefVan>` algorithm. It is part of :ref:`ILL's direct geometry reduction algorithms <DirectILL>`.

.. note::
    At the moment, the integration range is fixed to :math:`\pm` 3 * FWHM (:math:`2\sqrt{2 \ln 2}`) times the 'Sigma' column in *EPPWorkspace*).

Input workspaces
################

The *InputWorkspace* should be loaded using the :ref:`DirectILLCollectData <algm-DirectILLCollectData>` algorithm. It will also give the EPP workspace  needed for *EPPWorkspace*.

Vanadium temperature
####################

A correction for the Debye-Waller factor is applied to the integrated vanadium, as explained in the documentation of :ref:`ComputeCalibrationCoefVan <algm-ComputeCalibrationCoefVan>`. The temperature for the DWF calculation is taken from the 'Sample.temperature' sample log of the *InputWorkspace*. This value can be overridden by the *Temperature* property, if needed.

Usage
-----

For usage of this algorithm, check the examples :ref:`here <DirectILL>`.

.. categories::

.. sourcelink::
