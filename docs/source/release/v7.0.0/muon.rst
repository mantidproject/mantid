============
Muon Changes
============


Frequency Domain Analysis
-------------------------

Bugfixes
############


Muon Analysis
-------------

Bugfixes
############
- (`#41711 <https://github.com/mantidproject/mantid/pull/41711>`_) Added the ability to specify the location of the ``autosave.run`` (used for loading the current run) file in the Muon Analysis <Muon_Analysis-ref> and Frequency Domain Analysis <Frequency_Domain_Analysis-ref> interfaces. This allows users on platforms other than Windows to specify the location of the autosave file (location is automatically set on Windows).
- (`#42166 <https://github.com/mantidproject/mantid/pull/42166>`_) Fixed a bug in the :ref:`Muon Analysis <Muon_Analysis-ref>` interface where rapidly clicking the increment or decrement run arrows could cause the interface to hang. The same run could be loaded more than once at the same time, and the resulting loads could block each other indefinitely.


Muon Analysis and Frequency Domain Analysis
-------------------------------------------

Bugfixes
############
- (`#41902 <https://github.com/mantidproject/mantid/pull/41902>`_) Fixed a bug in the :ref:`Muon Analysis <Muon_Analysis-ref>` and :ref:`Frequency Domain Analysis <Frequency_Domain_Analysis-ref>` interfaces where entering an X (or Y) minimum value greater than or equal to the maximum in the plot range boxes flipped the axis. The invalid value is now rejected and the range reverted to its previous value.


ALC
---

Bugfixes
############


Elemental Analysis
------------------

Bugfixes
############


Algorithms
----------

Bugfixes
############

:ref:`Release 7.0.0 <v7.0.0>`
