=========================
Indirect Geometry Changes
=========================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

New features
============

- The Abins Algorithm can now import XML data from VASP calculations
  using "selective dynamics" to restrict the set of atoms active in
  vibrations. The data is imported and processed as though these are
  the only atoms in the system, with appropriately-dimensioned
  displacement data. This approximation is useful for the study of
  light (e.g. organic) molecules adsorbed to surfaces of heavy
  (e.g. noble-metal) catalysts.

:ref:`Release 6.2.0 <v6.2.0>`
