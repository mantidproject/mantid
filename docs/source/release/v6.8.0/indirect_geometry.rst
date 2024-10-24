=========================
Indirect Geometry Changes
=========================

.. contents:: Table of Contents
   :local:

New Features
------------
- When loading resolution data containing `NAN` values into :ref:`Data Analysis <interface-inelastic-qens-fitting>` they will be replaced with zeros and a warning displayed to the user.
- Added the quasielasticbayes package to the Linux standalone tarball.


Bugfixes
--------
- The data analysis GUI will no longer crash when using plot guess if the sample data contains NANs.
- Fix bug when choosing the :ref:`EISFDiffSphereAlkyl <func-EISFDiffSphereAlkyl>` algorithm in the F(Q) fit tab of the Indirect - Data Analysis window.


Algorithms
----------

New features
############
- Added :ref:`BayesQuasi2 <algm-BayesQuasi2>`, based on the quickBayes package, as a replacement for the now deprecated :ref:`BayesQuasi <algm-BayesQuasi>`.
- Added :ref:`BayesStretch2 <algm-BayesStretch2>`, based on the quickBayes package, as a replacement for the now deprecated :ref:`BayesStretch <algm-BayesStretch>`.

Bugfixes
############
- :ref:`Abins <algm-Abins>` now delegates parsing of CASTEP .phonon files to the Euphonic library.
  This gives more robust handling of data files containing Gamma-points with
  LO-TO splitting; the legacy parser skips past other q-points to find these points,
  leading to an unpredictable loss of information. Whereas the previous parser favours
  points with LO-TO splitting, this implementation instead favours non-split points
  where available; these should give better statistics. Note that very problematic .phonon
  files which were losing data away from Gamma are rare; typically CASTEP will include the
  Gamma points at the top, as assumed by the current behaviour.
- The Abins GAUSSIAN parser now reads "Standard Orientation"
  positions. This does not affect results but prevents errors when
  "Input Orientation" data is missing from the .log file.
- Support vasprun.xml files from Vasp 6 in Abins/:ref:`Abins2D <algm-Abins2D>`. The data
  units have changed since Vasp 5 from atomic units to THz; this
  format is now detected and an appropriate frequency conversion
  factor is used.
- Fixed a bin edge error and exception raised when performing an elastic reduction for a single scan with (:ref:`ElasticEMUauReduction <algm-ElasticEMUauReduction>`).

:ref:`Release 6.8.0 <v6.8.0>`
