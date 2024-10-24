========================
Direct Inelastic Changes
========================

.. contents:: Table of Contents
   :local:

Algorithms
----------

`Re#13566 <https://github.com/mantidproject/mantid/issues/13566>`__ New
:ref:`GetAllEi <algm-GetAllEi>`
algorithm analyses chopper logs and signals on monitors to identify all
possible incident energies used in Direct inelastic experiments in
single or multi-rep mode. Internally the algorithm incorporates four
sub-algorithms used by instrument scientists on instruments with
different background, chopper system and signal quality so it can
reliably run on all these instruments. The algorithm is the basis of
changes to autoreduction, which allow instrument scientists and users to
reduce (convert to SPE) results of an inelastic experiment without any
user intervention.

ISIS inelastic instruments descriptions have been modified to contain
chopper location and other additional information necessary to calculate
time, when chopper lets neutrons trough from appropriate chopper logs.

`Re#14071 <https://github.com/mantidproject/mantid/issues/14071>`_
:ref:`RenameWorkspace <algm-RenameWorkspace>` algorithm renames separate monitor workspace, if a
monitor workspace is attached to the current workspace. This allows
substantial simplification of the Direct Reduction workflow, where the
number of operations are performed on a workspace itself and the monitor
workspace, which is associated with the main workspace.

`Re#13980 <https://github.com/mantidproject/mantid/issues/13980>`_
On the basis of changes outlined in
`Re#13566 <https://github.com/mantidproject/mantid/issues/13566>`_ and
`Re#14071 <https://github.com/mantidproject/mantid/issues/14071>`_,
ISIS reduction now has the capability to identify incident energies used
in direct inelastic experiments and reduce data according to these
energies. **AUTO** key-word allow data reduction without providing a
guess for the incident energy. Reduction finishes as normal if no
energies were identified, returning the message stating the run is not
probably intended for reduction. This feature allows convenient use of
the ISIS autoreduction service to process data from direct inelastic
experiment without user or instrument scientist intervention.

ISIS Fixes
----------

`Re#14456 <https://github.com/mantidproject/mantid/issues/14456>`_. A
number of bug-fixes and small modifications were performed in order to
enable correct background removal for direct inelastic reduction.

-  The
   :ref:`CalculateFlatBackground <algm-CalculateFlatBackground>`
   algorithm has a new property ``NullifyNegativeValues`` which toggles
   nullification of negative signals after background subtraction.
-  The error calculation procedure used by the
   :ref:`RemoveBackground <algm-RemoveBackground>`
   algorithm has been replaced by the procedure used in
   :ref:`CalculateFlatBackground <algm-CalculateFlatBackground>` algorithm.
   The previous procedure received many complaints and was therefore
   deemed to be incorrect.
-  :ref:`RemoveBackground <algm-RemoveBackground>`
   has been modified to allow instrument scientists to nullify negative
   signals on request.
-  New property ``nullify_negative_signal`` has been added to ISIS
   reduction to toggle removal of negative signal according to
   particular requests. The default behaviour for nullifying negative
   signals has been defined in instrument definition files according to
   user preference.
-  The negative signal removal, if requested, now occurs after the
   spectra have been grouped according to the particular maps, which
   substantially improves statistics and final background in the case
   when negative signals are nullified.
-  Fixed bug causing reduction to fail if monitors are summed and
   monitor-1 normalisation is requested.

`Re#12403 <https://github.com/mantidproject/mantid/issues/12403>`_ LET
IDF has been modified using an embedded IDF correction mechanism which
was introduced in Mantid release 3.4. This mechanism allows users to
update the Instrument Definition (Files) (IDF) stored within nexus data
files. The exclusions based on run numbers, which had to be introduced
into ISIS direct inelastic reduction program were removed and this new
feature allows Instrument Definition information to be safely embedded
into the nexus files (or modified at a later stage).

Other fixes
-----------

-  Fixed problems with the direct geometry planning tool.
   `Re#15182 <https://github.com/mantidproject/mantid/issues/15182>`_

`Full list of changes on github <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.6%22+is%3Amerged+label%3A%22Component%3A+Direct+Inelastic%22>`_
