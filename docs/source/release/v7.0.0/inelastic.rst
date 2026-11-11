=================
Inelastic Changes
=================

New Features
------------
- (`#41781 <https://github.com/mantidproject/mantid/pull/41781>`_) The backend selection combo box has been removed from the :ref:`Inelastic Bayes Fitting interface<interface-inelastic-bayes-fitting>` following the removal of the ``quasielasticbayes`` library and the algorithms that depended on it. The output workspace names no longer contain a backend suffix.
- (`#41872 <https://github.com/mantidproject/mantid/pull/41872>`_) The :ref:`QENS Fitting interface<interface-inelastic-qens-fitting>` can now load workspaces from text files, e.g. from simulations, and fit them in the same way as workspaces loaded from Nexus files.
- (`#42105 <https://github.com/mantidproject/mantid/pull/42105>`_) The Symmetrise, Moments and Iqt tabs of the :ref:`Data Processor<interface-inelastic-data-processor>` interface can now load workspaces from text files, e.g. from simulations, in addition to Nexus files. The S(Q, w) and Elwin tabs are unaffected, as their Q-conversion algorithms require an instrument definition that a text file cannot provide.

Bugfixes
--------
- (`#41733 <https://github.com/mantidproject/mantid/pull/41733>`_) The :ref:`Moments<inelastic-moments>` tab of the :ref:`Data Processor<interface-inelastic-data-processor>` interface now correctly sets the ``EMin`` and ``EMax`` values for the *MomentsModel* when plotting new data, ensuring they are passed to the :ref:`SofQWMoments <algm-SofQWMoments>` algorithm initialization.
- (`#42351 <https://github.com/mantidproject/mantid/pull/42351>`_) The :ref:`Iqt<iqt>` tab of the :ref:`Data Processor<interface-inelastic-data-processor>` interface now logs a warning to the console instead of displaying a warning message box when the binning is less than 5.

:ref:`Release 7.0.0 <v7.0.0>`
