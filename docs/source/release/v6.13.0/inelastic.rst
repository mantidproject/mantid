=================
Inelastic Changes
=================

.. contents:: Table of Contents
   :local:

New Features
------------
- Added new `TeixeiraWaterIqtFT` fitting function, which is the Fourier Transform of `TeixeiraWaterIqt` function. This is now available in the ConvFit tab of the :ref:`QENS Fitting <interface-inelastic-qens-fitting>` interface.


Bugfixes
--------
- :ref:`QENSFitting <interface-inelastic-qens-fitting>` interface will generate a group workspace after fitting with suffix `_Result` when the output fit is a single fit, and suffix `_Results` when is a multiple fit.
- Clicking plot button after a fit from :ref:`QENSFitting <interface-inelastic-qens-fitting>` no longer plot results twice.


Algorithms
----------

New features
############
- :ref:`BayesQuasi2 <algm-BayesQuasi2>` and :ref:`BayesStretch2 <algm-BayesStretch2>` are now available for all Mantid users now that ``quickbayes`` v1.0.2 is installed as a ``mantid`` dependency via Conda.

Bugfixes
############


:ref:`Release 6.13.0 <v6.13.0>`
