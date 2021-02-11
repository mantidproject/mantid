============
SANS Changes
============

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

Algorithms and instruments
--------------------------

Improvements
############

- :ref:`SANSILLReduction <algm-SANSILLReduction>` has a new property `SolventInputWorkspace`, to provide
  reduced solvent data to be subtracted from the sample data.
- :ref:`SANSILLAutoProcess <algm-SANSILLAutoProcess>` has a new property `SolventFiles`, to communicate
  with :ref:`SANSILLReduction <algm-SANSILLReduction>` the file names of the reduced solvent data.

Bugfixes
########


:ref:`Release 6.1.0 <v6.1.0>`
