.. _MaskWorkspace:

MaskWorkspace
=============

.. contents::
  :local:

**NOTE: This document is may require additional work**

What is it?
-----------

A :py:obj:`mantid.dataobjects.MaskWorkspace` is a :py:obj:`mantid.dataobjects.SpecialWorkspace2D`
that may be applied to another Workspace in order to non-destructively exclude data from future calculations.
It may be reused to apply masking to additional Workspaces as applicable to your workflow.

How Does it Work?
-----------------

It does so by mapping :term:`Detector ID <Detector ID>`'s to a boolean value.

`0` = Unmasked = LIVE_VALUE

`1` = Masked = DEAD_VALUE


This boolean is stored in the first and only y-value of one of many spectra *initially* mapped 1:1 to the Detector ID's of an Instrument. 😐

This is per the implementation of :py:obj:`mantid.dataobjects.SpecialWorkspace2D`.

**NOTE:** This mapping is not *guaranteed* to always be 1:1, as the mapping allows multiple Detector ID to be associated with the same Spectrum.
However, it is uncommon to group a mask's detector mapping and the MaskWorkspace interface has no methods supporting such an operation.
(*An example of such a usecase would improve this article. Otherwise MaskWorkspace should actively enforce a 1:1 policy.*)

This mapping seems odd at first, but serves a pragmantic purpose when interfacing with the rest of Workbench.
*Opinion:* This implementation conflates frontend/backend representation of the data by modeling its format after what a user might expect particle data to be in.
E.g. Each Detector has a Spectrum with data recorded over some unit, but for a SpecialWorkspace2D this value would be constant.


The **X Data/Y Data** of the MaskWorkspace should be the **only source of truth** when determining if a Detector ID is masked.

However, as a :py:obj:`mantid.dataobjects.SpecialWorkspace2D`, it also has a SpectrumInfo->DectectorInfo.

This DetectorInfo also has the capability to track whether or not a given Detector ID is masked.😐

The SpectrumInfo *also* can track if a Detector ID is masked but becomes unreliable for more complex mappings.

**Don't use either of the above.^**

These objects serve as metadata about the Workspace.
On any other Workspace, these determine if specific Detector IDs or Spectrum are masked.
In the case of a MaskWorkspace, relying on these instead of the main data creates indirection and uncertainty.
*Opinion:* The masking aspect of DetectorInfo and SpectrumInfo probably shouldnt have been included on MaskWorkspace to begin with.


**NOTE:** Typically, throughout the codebase if a Detector ID *does not exist* in a MaskWorkspace, it is considered *Masked*.
However, it is safer to fully define and validate that your MaskWorkspace has a value assigned for each Detector ID of your target workspace.
*Opinion:* There may be an opportunity for performance improvement if instead a mask only held Detector IDs it masked, e.g. the opposite of what is currently assumed.
Or had an option to invert what its values meant.


Implementation Notes
--------------------

**This document was not written by the author of MaskWorkspace.  These notes may border on speculation.**

MaskWorkspace/SpecialWorkspace2D contains a map member variable `detID_to_WI`.
This member serves in part as a cache for the map that would normally be generated as part of `MatrixWorkspace::getIndicesFromDetectorIDs`.
This is used to track which Detector IDs are associated with which Spectrum.
I do not reccommend doing so, but this mapping supports assiging multiple Detector IDs to the same Spectrum.
This is a byproduct of being a :py:obj:`mantid.dataobjects.SpecialWorkspace2D`, and is applicable in other child classes such as :py:obj:`mantid.dataobjects.GroupingWorkspace`,
If this sounds useful, maybe MaskWorkspace could be expanded to support such behavior with additional context beyond just this arbitrary map.
But currently, it is usually expected to have a 1:1 mapping of Detector ID to Spectrum

This implementation mirrors what DetectorInfo already does except you have the overhead of this map variable (and all the metadata associated with workspaces and specta).

DetectorInfo instead associates a Detector ID with an arbitrary index in a vector.
This index then serves as a key to access several other vectors that store property information associate with the detector(like if it is masked).
This makes DetectorInfo lighter on memory but lookups based on Detector ID require an initial search to find the appropriate index.
*(A cost-benefit analysis with timing and memory statistics of this implementation vs a map of detector objects would improve this doc.)*
*(e.g. It seems like we are already creating overhead, as `m_isMasked.size()`, `m_isMonitor.size()` seems to equal the total number of positions,
despite the normal case in which they are actually significantly smaller in size. (Framework/Beamline/src/DetectorInfo.cpp))*

A Detector ID is then just a unique user-facing name of a Detector, and the backend ID is just an index.
So really, a Detector ID is more of a property on a Detector rather than a real identifier.
This is further implied, because certain properties of a Detector ID value can denote a certain kind of detector, for instance if it is negative.
An actual ID is agnostic of the set of properties is represents.
*(This doc could use a note explaining why Detector ID as a concept is necessary. I think it has to do with how real Instruments handle enumerating their detectors)*

Thus, a MaskWorkspace then must serve **both** as the **front end interface** mapping Detector ID to Detector index,
and as the **backend data** used to apply masking to Workspaces.


Its also worth noting that each spectrum has a SpectrumDefinition, which contains the *indexes* of each Detector ID associated with it.
It is the mapping of these indexes to a DetectorInfo that determines if a whole Spectrum is masked implictly.


**NOTE:** There actually *does seem to exist* a `m_detIDToIndex` map member on DetectorInfo that is seemingly only initialized and used by :ref:`algm-SaveDiffCal`.
So hypothetically, one might move the generation of this map to DetectorInfo itself, and use that to backend both SaveDiffcal and SpecialWorkspace2D.
(Framework/Geometry/src/Instrument/DetectorInfo.cpp)
