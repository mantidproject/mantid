.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm will flag the detectors listed as
masked(\ `IDetector <IDetector>`__ isMasked() method) and will zero the
data in the spectra related to those detectors.

All but the first property are optional and at least one of the must be
set. If several are set, the first will be used.

The set of detectors to be masked can be given as a list of either
spectrum numbers, detector IDs or workspace indices. The list should be
set against the appropriate property.

Mask Detectors According To Instrument
######################################

If the input MaskedWorkspace is not a SpecialWorkspace2D object, this
algorithm will check every detectors in input MaskedWorkspace's
Instrument. If the detector is masked, then the corresponding detector
will be masked in Workspace.

Mask Detectors According to Masking Workspace
#############################################

If the input MaskedWorkspace is a `MaskWorkspace <MaskWorkspace>`__
object, i.e., masking workspace, then the algorithm will mask
Workspace's detector according to the histogram data of the
SpecialWorkspace2D object

Definition of Mask
##################

-  If a pixel is masked, it means that the data from this pixel won't be
   used. In the masking workspace (i.e.,
   `SpecialWorkspace2D <SpecialWorkspace2D>`__), the corresponding value
   is 1.
-  If a pixel is NOT masked, it means that the data from this pixel will
   be used. In the masking workspace (i.e.,
   `SpecialWorkspace2D <SpecialWorkspace2D>`__), the corresponding value
   is 0.

About Input Parameters
######################

:ref:`algm-MaskDetectors` supports various format of input to
mask detectors, including

-  Workspace indices
-  Spectra
-  Detectors
-  `MaskWorkspace <MaskWorkspace>`__
-  General `MatrixWorkspace <MatrixWorkspace>`__ other than
   `MaskWorkspace <MaskWorkspace>`__ (In this case, the mask will be
   extracted from this workspace)

Rules
#####

Here are the rules for input information for masking

| ``1. At least one of the inputs must be specified.   ``
| ``2. Workspace indices and Spectra cannot be given at the same time. ``
| ``3. ``\ ```MaskWorkspace`` <MaskWorkspace>`__\ `` and general ``\ ```MatrixWorkspace`` <MatrixWorkspace>`__\ `` cannot be given at the same time. ``
| ``4. When a general ``\ ```MatrixWorkspace`` <MatrixWorkspace>`__\ `` is specified, then all detectors in a spectrum are treated as masked if the effective detector of that spectrum is masked. ``
| ``5. The masks specified from ``
| ``  a) workspace indices/spectra``
| ``  b) detectors``
| ``  c) ``\ ```MaskWorkspace`` <MaskWorkspace>`__\ ``/general ``\ ```MatrixWorkspace`` <MatrixWorkspace>`__
| ``  will be combined by the ``\ *``plus``*\ `` operation.``

Operations Involved in Masking
##############################

There are 2 operations to mask a detector and thus spectrum related

| ``1. Set the detector in workspace's instrument's ``\ *``parameter``
``map``*\ `` to ``\ *``masked``*\ ``;``
| ``2. Clear the data associated with the spectrum with detectors that are masked;``

Implementation
##############

In the plan, the workflow to mask detectors should be

| ``1. Convert input detectors, workspace indices or spectra, and general ``\ ```MatrixWorkspace`` <MatrixWorkspace>`__\ `` to a ``\ ```MaskWorkspace`` <MaskWorkspace>`__\ ``;``
| ``2. Mask detectors according to ``\ ```MaskWorkspace`` <MaskWorkspace>`__\ ``;``
| ``3. Clear data on all spectra, which have at least one detector that is masked.``

Concern
#######

-  Speed!

.. categories::
