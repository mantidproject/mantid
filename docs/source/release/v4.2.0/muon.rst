============
MuSR Changes
============

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

Algorithms
----------

Improvements
############

- Improve the handling of :ref:`LoadPSIMuonBin<algm-LoadPSIMuonBin-v1>` where a poor date is provided.

Interfaces
----------

Muon Analysis 2
###############

- When loading PSI data if the groups given are poorly stored in the file, it should now produce unique names in the grouping tab for groups.

:ref:`Release 4.2.0 <v4.2.0>`