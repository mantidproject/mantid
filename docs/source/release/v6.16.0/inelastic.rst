=================
Inelastic Changes
=================

New Features
------------
- (`#41228 <https://github.com/mantidproject/mantid/pull/41228>`_) The :ref:`Absorption Corrections<absorption_corrections>` tab now has an improved UI which displays existing elements better and adds new features.

Deprecated
----------
- (`#41712 <https://github.com/mantidproject/mantid/pull/41712>`_) This release will be the final version to include ``quasielasticbayes`` support.
  The :ref:`Bayes Fitting interface <interface-inelastic-bayes-fitting>` now uses ``quickbayes`` (the new python library) by default.
  In the next release the option to use ``quasielasticbayes`` will be removed.
  The :ref:`algm-BayesQuasi` and :ref:`algm-BayesStretch` algorithms (based on ``quasielasticbayes``) have been deprecated in favour of the ``quickbayes`` replacements, :ref:`algm-BayesQuasi2` and :ref:`algm-BayesStretch2`, respectively.

Bugfixes
--------
- (`#41250 <https://github.com/mantidproject/mantid/pull/41250>`_) The :ref:`Stretch <bayes-fitting-stretch>` tab now verifies workspaces have more than one bin before creating :ref:`Colorfill_Plots`.
- (`#41228 <https://github.com/mantidproject/mantid/pull/41228>`_) The :ref:`Container Subtraction <container-subtraction>` interface no longer deletes the subtracted workspace from the ADS when the window is closed.
- (`#41228 <https://github.com/mantidproject/mantid/pull/41228>`_) The :ref:`Corrections <interface-inelastic-corrections>` interface no longer hangs on close after using a container-subtracted workspace in the :ref:`Apply Absorption corrections <apply_absorp_correct>` tab.
- (`#41228 <https://github.com/mantidproject/mantid/pull/41228>`_) The :ref:`Container Subtraction <container-subtraction>` interface now includes an output-name widget for labelling outputs in the ADS.


Algorithms
----------

New features
############

Bugfixes
############
- (`#41121 <https://github.com/mantidproject/mantid/pull/41121>`_) The :ref:`BayesStretch <algm-BayesStretch>` algorithm now appends each workspace name in the fit workspaces group with the selected backend option of either ``quickbayes`` or ``quasielasticbayes`` from the :ref:`Stretch <bayes-fitting-stretch>` tab.
- (`#41121 <https://github.com/mantidproject/mantid/pull/41121>`_) Additionally, a duplicate ``Stretch`` word has been removed from the names of workspaces in the fit workspace group created by the :ref:`BayesStretch2 <algm-BayesStretch2>` algorithm when the selected backend option is ``quickbayes`` in the :ref:`Stretch <bayes-fitting-stretch>` tab.

:ref:`Release 6.16.0 <v6.16.0>`
