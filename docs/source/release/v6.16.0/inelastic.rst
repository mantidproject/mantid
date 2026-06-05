=================
Inelastic Changes
=================

New Features
------------
- (`#41228 <https://github.com/mantidproject/mantid/pull/41228>`_) Improve UI layout in :ref:`Absorption Corrections<absorption_corrections>` tab.


Bugfixes
--------
- (`#41250 <https://github.com/mantidproject/mantid/pull/41250>`_) The :ref:`Bayes Fitting interface <interface-inelastic-bayes-fitting>` ``Stretch`` tab now verifies that workspaces have more than one bin before creating :ref:`Colorfill_Plots`.
- (`#41228 <https://github.com/mantidproject/mantid/pull/41228>`_) The :ref:`Container Subtraction <container-subtraction>` interface no longer deletes the subtracted workspace from the ADS when the window is closed.
- (`#41228 <https://github.com/mantidproject/mantid/pull/41228>`_) The :ref:`Corrections <interface-inelastic-corrections>` interface no longer hangs on close after using a container-subtracted workspace in the :ref:`Apply Absorption corrections <apply_absorp_correct>` tab.
- (`#41228 <https://github.com/mantidproject/mantid/pull/41228>`_) The :ref:`Container Subtraction <container-subtraction>` interface now includes an output-name widget for labelling outputs in the ADS.


Algorithms
----------

New features
############

Bugfixes
############
- (`#41121 <https://github.com/mantidproject/mantid/pull/41121>`_) The :ref:`BayesStretch <algm-BayesStretch>` algorithm was updated to include the backend name to the name of each workspace in its fit workspaces group in ``Stretch`` tab of :ref:`Bayes Fitting interface <interface-inelastic-bayes-fitting>`
- (`#41121 <https://github.com/mantidproject/mantid/pull/41121>`_) In addition a duplicate ``Stretch`` word was removed from names of workspaces in the fit workspace group resulting from :ref:`BayesStretch2 <algm-BayesStretch2>` when the backend is set to ``quickbayes`` in ``Stretch`` tab.

:ref:`Release 6.16.0 <v6.16.0>`
