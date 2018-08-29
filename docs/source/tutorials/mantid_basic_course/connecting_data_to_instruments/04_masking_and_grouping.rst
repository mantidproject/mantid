.. _04_masking_and_grouping:

====================
Masking And Grouping 
====================

The Draw tab is for grouping or masking detectors.

The toolbar on this tab consists of the same tools as the top row of
tools in the Pick tab. Their functions and behaviours are also the same
but the shapes are used for masking and grouping instead of displaying
the data.

.. figure:: /images/MaskTabToolButtons.png
   :alt: MaskTabToolButtons.png

Use these radio-buttons to switch between masking and grouping
functions.

.. figure:: /images/MaskOrGroup.png
   :alt: MaskOrGroup.png

The shapes ready for masking have red border |MaskRedRing.png|

The shapes ready for grouping have blue border |GroupBlueRing.png|

Grouping
========

In the grouping mode you can extract the data or save grouping to a
file:

.. figure:: /images/GroupingOperations.png
   :alt: GroupingOperations.png

Masking
=======

In the masking mode operations can either be applied to the view only or
to the underlying workspace.

.. figure:: /images/MaskingViewAndWorkspace.png
   :alt: MaskingViewAndWorkspace.png

When the "Apply to View" button is clicked the detectors covered by the
shapes are greyed out indicating that they are masked.

.. figure:: /images/MaskedViewPixels.png
   :alt: MaskedViewPixels.png

The purpose of applying to view only is to make creation of masking
workspaces/files easier. You can create a mask, apply it to view, save
it in a workspace or a file but then clear the view
(|ClearAllButton.png| button) and create another mask without closing
the Instrument View or need to reload the data. The masking operations
are available from the "Apply and Save" menu.

.. figure:: /images/ApplyAndSaveMenu.png
   :alt: ApplyAndSaveMenu.png

If masking is applied to the data (by clicking |ApplyToDataButton.png|)
it cannot be undone.

.. raw:: mediawiki

   {{SlideNavigationLinks|MBC_Investigating_Data|Mantid_Basic_Course|MBC_Instrument_Tree}}

.. |MaskRedRing.png| image:: /images/MaskRedRing.png
.. |GroupBlueRing.png| image:: /images/GroupBlueRing.png
.. |ClearAllButton.png| image:: /images/ClearAllButton.png
.. |ApplyToDataButton.png| image:: /images/ApplyToDataButton.png
