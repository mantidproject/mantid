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
- :ref:`LoadILLSANS <algm-LoadILLSANS>` is extended to support loading monochromatic kinetic files from the new D11.
- :ref:`SANSILLReduction <algm-SANSILLReduction>` is extended to support processing of monochromatic kinetic runs.
- A series of improvements have been introduced to :ref:`SANSILLAutoProcess <algm-SANSILLAutoProcess>` such as the use of the new :ref:`Stitch <algm-Stitch>`, allowing for semi-automatic q binning, supporting attenuator 2 on D22, as well as improved naming and grouping for the output workspaces.

Bugfixes
--------


Improvements
############

- :ref:The ANSTO Bilby loader `LoadBBY <algm-LoadBBY>` logs the occurence of invalid events detected in the file as a warning.

:ref:`Release 6.2.0 <v6.2.0>`
