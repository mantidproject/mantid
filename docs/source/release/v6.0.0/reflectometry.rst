=====================
Reflectometry Changes
=====================

.. contents:: Table of Contents
   :local:

ISIS Reflectometry Interface
############################

- **Runs in the search results list can now be excluded or annotated, e.g. to exclude runs from autoprocessing. See the**
  :ref:`ISIS Reflectometry Interface <interface-isis-refl>` **documentation for details.**

.. figure:: /images/ISISReflectometryInterface/transfer.png
   :class: screenshot
   :width: 650px
   :align: center
   :alt: Selecting runs from search table to transfer to processing table

- A big has been fixed where the flood workspace drop-down box was being accidentally populated with the first workspace
  created after clearing the workspaces list.

Algorithms
##########

- A bug has been fixed where ``import CaChannel`` on Linux would cause a hard crash.
- A bug has been fixed where  running :ref:`algm-ReflectometryReductionOneAuto` on workspace groups resulted in spurious
  error messages about transmission workspaces.

:ref:`Release 6.0.0 <v6.0.0>`
