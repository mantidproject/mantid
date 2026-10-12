=================
Inelastic Changes
=================

New Features
------------
- (`#41781 <https://github.com/mantidproject/mantid/pull/41781>`_) Removed the backend selection combo box from the :ref:`Inelastic Bayes Fitting interface <interface-inelastic-bayes-fitting>` after the ``quasielasticbayes`` library (and the algorithms which depended on it) was removed. The output workspaces no longer have a suffix indicating which backend was used to create them.
- (`#41872 <https://github.com/mantidproject/mantid/pull/41872>`_) The QENS Fitting interface can now load workspaces from text files, e.g. from simulations, and perform fits as with Nexus files.
- (`#42105 <https://github.com/mantidproject/mantid/pull/42105>`_) The Inelastic Data Processor interface's Symmetrise, Moments and Iqt tabs can now load workspaces from text files, e.g. from simulations, in addition to Nexus files. The Sqw and Elwin tabs are unaffected, as their Q-conversion algorithms require an instrument definition that a text file cannot provide.


Bugfixes
--------
- (`#41733 <https://github.com/mantidproject/mantid/pull/41733>`_) The :ref:`Moments<inelastic-moments>` tab now correctly sets the ``EMin`` and ``EMax`` value for the *MomentsModel* when plotting new data so that it is correctly propagated through to the :ref:`SofQWMoments <algm-SofQWMoments>` algorithm initialization.
- (`#42351 <https://github.com/mantidproject/mantid/pull/42351>`_) The :ref:`Iqt<iqt>` tab of the :ref:`Data Processor<interface-inelastic-data-processor>` interface no longer pops up a warning message box when binning is less than 5, instead it shows the warning message on the console log.


Algorithms
----------

New features
############

Bugfixes
############

:ref:`Release 7.0.0 <v7.0.0>`
