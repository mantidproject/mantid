=================
Framework Changes
=================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

Concepts
--------

Algorithms
----------
:ref:`LoadNexusLogs <algm-LoadNexusLogs>` has additional parameters to allow or block specific logs from being loaded.
:ref:`LoadEventNexus <algm-LoadEventNexus>` now utilizes the log filter provided by `LoadNexusLogs <algm-LoadNexusLogs>`.

- :ref:`CompareWorkspaces <algm-CompareWorkspaces>` compares the positions of both source and sample (if extant) when property `checkInstrument` is set.
- :ref:`SetGoniometer <algm-SetGoniometer>` can now set multiple goniometers from log values instead of just the time-avereged value.
- :ref:`SaveAscii <algm-SaveAscii>` can now create a header for the output file containing sample logs specified through the new property `LogList`.

Data Objects
------------

- :ref:`Run <mantid.api.Run>` has been modified to allow multiple goniometers to be stored.

Python
------


.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

Installation
------------


MantidWorkbench
---------------

See :doc:`mantidworkbench`.

SliceViewer
-----------

Improvements
############

Bugfixes
########

- Fix problem with dictionary parameters on :ref:`SetSample <algm-SetSample>` algorithm when running from the algorithm dialog

:ref:`Release 6.1.0 <v6.1.0>`
