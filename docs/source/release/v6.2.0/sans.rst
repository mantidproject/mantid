============
SANS Changes
============

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

New
---

- :ref:`SANSILLIntegration <algm-SANSILLIntegration>` has new resolution calculation option alternative to Mildner-Carpenter based on fitting horizontal size of direct beam. The fitting is handled in :ref:`SANSILLReduction <algm-SANSILLReduction>` while processing beam.


Bugfixes
--------

- The ISIS SANS Interface will now display an error, instead of an unexpected error, if the user does not have permission to save a CSV file to the requested location.

Improvements
############

- :ref:The ANSTO Bilby loader `LoadBBY <algm-LoadBBY>` logs the occurence of invalid events detected in the file as a warning.

:ref:`Release 6.2.0 <v6.2.0>`
