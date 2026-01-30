=================
Inelastic Changes
=================

.. contents:: Table of Contents
   :local:

New Features
------------
- (`#40595 <https://github.com/mantidproject/mantid/pull/40595>`_) Added a combo box to the :ref:`Bayes Fitting interface <interface-inelastic-bayes-fitting>` to swap between using the ``quasielasticbayes`` backend and the ``quickbayes`` backend. Changing this box won't effect the functionality of the ResNorm tab, which already uses ``quickbayes``.


Bugfixes
--------


Algorithms
----------

New features
############
- (`#40323 <https://github.com/mantidproject/mantid/pull/40323>`_) A new algorithm :ref:`SpectralMomentMD <algm-SpectralMomentMD>` can now be used to calculate sum rules for inelastic multi-dimensional event workspaces.

Bugfixes
############
- (`#40650 <https://github.com/mantidproject/mantid/pull/40650>`_) ref:`<algm-BayesQuasi>` now respects the ``OutputWorkspaceProb`` and ``OutputWorkspaceResult`` parameters.

:ref:`Release 6.15.0 <v6.15.0>`
