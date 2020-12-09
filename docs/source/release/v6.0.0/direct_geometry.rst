=======================
Direct Geometry Changes
=======================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

Improvements
------------

- The instrument geometry of PANTHER has been corrected according to the findings during the hot commissioning.
- New IDF has been added for the SHARP time-of-flight spectrometer at the ILL.

ALF View
--------

New
###
- An estimation of fit parameters is performed when extracting a single tube for the first time. The estimate can subsequently be
  updated using the `Update Estimate` button.


CrystalField
------------

New
###
- Extended the ``Background`` class to accept a list of functions through using the ``functions`` keyword. This
  allows more than two functions to be used for the ``Background`` of a CrystalField fit as seen in the example below.

.. code-block:: python

    cf.background = Background(functions=[Function('Gaussian',Sigma=0.05313/2, Height= 1279300,PeakCentre=-0.0021),
                                          Function('Gaussian',Sigma=0.1111/2, Height= 1465,PeakCentre=0.09147),
                                          Function('LinearBackground', A0=34, A1=0.01)])

    # Ties can then be applied by indexing to the relevant function
    cf.background.functions[1].ties(Height=1465, Sigma=0.1111/2, PeakCentre=0.09147)

- Implemented a method for fixing function parameters in the ``Background`` to their current values. An example using
  the same background as above is:

.. code-block:: python

    # Fixes the PeakCentre and Height of the first Gaussian to their current values.
    cf.background.functions[0].fix('PeakCentre', 'Height')

    # Fixes all the parameters of the LinearBackground to their current values.
    cf.background.functions[2].fix('all')

BugFixes
########
- Fixed a bug in the :ref:`Crystal Field Python Interface` where ties were not being applied properly for cubic crystal structures.
- Fixed a bug in the :ref:`LoadCIF <algm-LoadCIF>` algorithm caused when a **.cif** file has 2 sections, with the first not having
  the required data keys.


DGSPlanner
----------

Improvements
############

- Widgets were rearranged into groups of items based on their logical function

:ref:`Release 6.0.0 <v6.0.0>`
