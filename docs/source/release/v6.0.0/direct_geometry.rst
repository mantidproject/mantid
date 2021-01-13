=======================
Direct Geometry Changes
=======================

.. contents:: Table of Contents
   :local:

Improvements
------------

- The instrument geometry of PANTHER has been corrected according to the findings during the hot commissioning.
- New IDF has been added for the SHARP time-of-flight spectrometer at the ILL.

ALF View
--------

New
###
- An estimation of fit parameters is performed when extracting a single tube for the first time. The estimate can
  subsequently be updated using the `Update Estimate` button.


CrystalField
------------

New
###

- The ``Background`` class can now accept a list of functions through the ``functions`` keyword. This allows more than
  two functions to be used for the ``Background`` of a CrystalField fit.
- Function parameters in the ``Background`` of a CrystalField fit can be fixed to their current values.

.. code-block:: python

    cf.background = Background(functions=[Function('Gaussian',Sigma=0.05313/2, Height= 1279300,PeakCentre=-0.0021),
                                          Function('Gaussian',Sigma=0.1111/2, Height= 1465,PeakCentre=0.09147),
                                          Function('LinearBackground', A0=34, A1=0.01)])

    # Ties can then be applied by indexing to the relevant function
    cf.background.functions[1].ties(Height=1465, Sigma=0.1111/2, PeakCentre=0.09147)
    # Fixes the PeakCentre and Height of the first Gaussian to their current values.
    cf.background.functions[0].fix('PeakCentre', 'Height')
    # Fixes all the parameters of the LinearBackground to their current values.
    cf.background.functions[2].fix('all')

BugFixes
########
- A bug has been fixed in the :ref:`Crystal Field Python Interface` where ties were not being applied properly for cubic
  crystal structures.
- A bug has been fixed in the :ref:`CrystalFieldFunction <func-CrystalFieldFunction>` which prevented it working with
  the Trust Region minimizer.
- A bug has been fixed in the :ref:`LoadCIF <algm-LoadCIF>` algorithm caused when a **.cif** file has 2 sections, with
  the first not having the required data keys.


DGSPlanner
----------

Improvements
############

- Widgets have been rearranged into groups of items based on their logical function


MSlice
------

BugFixes
########
- A bug has been fixed prevented the selection of more than one workspace for workspace addition.
- A bug has been fixed in MSlice that caused a crash by ignoring the workspace selected by default for subtracting
  workspaces.
- A bug has been fixed in MSlice that caused a crash when entering unexpected values into the width box for cuts.

:ref:`Release 6.0.0 <v6.0.0>`
