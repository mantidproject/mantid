========================
Direct Inelastic Changes
========================

.. contents:: Table of Contents
   :local:

Algorithms
----------

New
###

- :ref:`CreateEPP <algm-CreateEPP>`: generate EPP tables compatible with :ref:`FindEPP <algm-FindEPP>` directly from instrument geometry.

Improved
########

- :ref:`LoadILLTof <algm-LoadILLTOF-v2>` now loads the monitor spectra last making the spectrum numbers and detectors IDs match.
- In :ref:`CorrectTOFAxis <algm-CorrectTOFAxis>`, the sample to detector distance can now be directly given using the new *L2* property.

Deprecated
##########

- The already deprecated version 1 of LoadILLTOF has been removed completely. Use :ref:`LoadILLTOF version 2 <algm-LoadILLTOF-v2>` instead.

Crystal Field
-------------

- Now accepts arbitrary `J` (or `S` - angular momentum quantum number, determining the basis states) 
  values with the syntax: `Ion=S<n>` or `Ion=J<n>` where `<n>` is an integer or half integer.

`Full list of changes on GitHub <https://github.com/mantidproject/mantid/issues?q=is%3Aclosed+milestone%3A%22Release+3.10%22+label%3A%22Component%3A+Direct+Inelastic%22>`_
