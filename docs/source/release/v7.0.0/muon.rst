============
Muon Changes
============

New features
############
- (`#41711 <https://github.com/mantidproject/mantid/pull/41711>`_) The location of the ``autosave.run`` file, used to load the current run, can now be specified in the :ref:`Muon Analysis <Muon_Analysis-ref>` and :ref:`Frequency Domain Analysis <Frequency_Domain_Analysis-ref>` interfaces. On Windows, the location is set automatically.

Bugfixes
############
- (`#42166 <https://github.com/mantidproject/mantid/pull/42166>`_) The :ref:`Muon Analysis <Muon_Analysis-ref>` interface no longer hangs when the run increment or decrement arrows are clicked rapidly, which could trigger simultaneous loads of the same run that blocked each other indefinitely.
- (`#41902 <https://github.com/mantidproject/mantid/pull/41902>`_) The plot range boxes in the :ref:`Muon Analysis <Muon_Analysis-ref>` and :ref:`Frequency Domain Analysis <Frequency_Domain_Analysis-ref>` interfaces now reject minimum X or Y values greater than or equal to the corresponding maximum, reverting to the previous value instead of flipping the axis.

:ref:`Release 7.0.0 <v7.0.0>`
