=============
Muon Analysis
=============

.. contents:: Table of Contents
   :local:

Interfaces
----------
Muon Analysis
-  The new algorithms :ref:`EstimateMuonAsymmetryFromCounts <algm-EstimateMuonAsymmetryFromCounts-v1>`: and :ref:`CalculateMuonAsymmetry <algm-CalculateMuonAsymmetry-v1>` are now used in the muon analysis GUI.
-  The main part of the multiple fitting GUI has been upgraded to be more user friendly.


- Fixed a bug that meant transverse field asymmetry data was normalized to bin width. 

- Added a fix normalization tick box into the TF asymmetry mode. Once this box is ticked (true) the normalization cannot be changed and will be applied to all of the data that is loaded into the GUI. 

Algorithms
----------
-  :ref:`EstimateMuonAsymmetryFromCounts <algm-EstimateMuonAsymmetryFromCounts-v1>`: new algorithm to estimate the asymmetry.
-  :ref:`CalculateMuonAsymmetry <algm-CalculateMuonAsymmetry-v1>`: new algorithm to calculate the asymmetry by using a fitting function.

Bug Fixes
---------
- Mantid would sometimes crash when the user was adding groups/pairs. This has been fixed by removing the automatic plotting from the group/pairs tab.


|

`Full list of changes <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.10%22+is%3Amerged+label%3A%22Component%3A+Muon%22>`_
on GitHub.
