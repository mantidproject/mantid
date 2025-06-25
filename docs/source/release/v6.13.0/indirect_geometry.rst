=========================
Indirect Geometry Changes
=========================

.. contents:: Table of Contents
   :local:

New Features
------------
- :ref:`algm-VesuvioPeakPrediction` now supports a ``Classical`` option. It is used to estimate the lower bounds of the
  width of the Neutron Compton Profiles when running analysis routines.
- :ref:`ISIS Energy Transfer tab <ISISEnergyTransfer>` now supports an output label that is appended to the output name
  and doesn't override previous outputs.

Algorithms
----------

New features
############
- The ``Atoms`` field of :ref:`algm-Abins` and :ref:`algm-Abins2D` algorithms has been enhanced to support more kinds of
  input.  Individual atoms may still be selected with e.g. ``atom1`` or ``atom_1``, but ``1`` is now also a valid input
  to select the first atom. In addition, ranges can be specified as e.g. ``2-4`` or ``2..4``. This makes the user input
  more concise, but will still create individual workspaces for atoms ``2``, ``3``, and ``4``.
- A ``CacheDirectory`` parameter has been added to the :ref:`algm-Abins` and :ref:`algm-Abins2D` algorithms. This allows
  an explicit cache directory to be set independently for each calculation; the previous behaviour (to use the User Save
  Directory) remains available as the default value.

:ref:`Release 6.13.0 <v6.13.0>`
