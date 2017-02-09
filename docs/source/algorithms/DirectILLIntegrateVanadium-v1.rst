.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm integrates the workspace given in *InputWorkspace* using the :ref:`ComputeCalibrationCoefVan <algm-ComputeCalibrationCoefVan>` algorithm.

.. note::
    At the moment, the integration range is fixed to :math:`\pm` 3 * FWHM (:math:`2\sqrt{2 \ln 2}` times the 'Sigma' column in *EPPWorkspace*).

Input workspaces
################

The *InputWorkspace* should be loaded using the :ref:`DirectILLPrepareData <algm-DirectILLPrepareData>` algorithm. It will also give the EPP workspace  needed for *EPPWorkspace*.

Usage
-----

**Example - Not implemented**

.. categories::

.. sourcelink::
