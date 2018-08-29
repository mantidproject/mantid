.. _03_investigating_data:

==================
Investigating Data 
==================

The Pick tab on the controls panel allows you to see the data in the
workspace.

The Toolbar
===========

| |PickTabToolButtons.png|

- |PickTabZoomButton.png| - Navigate in the instrument display window.
- |PickTabEditButton.png| - Edit a shape.
- |PickTabEllipseButton.png| - Draw an ellipse.
- |PickTabRectButton.png| - Draw a rectangle.
- |PickTabElRingButton.png| - Draw an elliptical ring.
- |PickTabRectRingButton.png| - Draw a rectangular ring.
- |PickTabPickPixelButton.png| - Select a single detector.
- |PickTabPickTubeButton.png| - Select a tube/bank.
- |PickTabAddPeakButton.png| - Add a single crystal peak.
- |PickTabEraseButton.png| - Erase a peak.

Picking a Single Detector
=========================

The Single Pixel tool |PickSingleDetector.png| displays the detector
data in the mini-plot at the bottom of the tab.

.. figure:: /images/Miniplot.png
   :alt: Miniplot.png

Hover the mouse over a detector and see the mini-plot update.

Picking a Tube
==============

The Tube selection tool |PickTube.png| is useful for tube instruments.
When it's on the mini-plot displays the integrated data in the whole
tube. The integration is done either over the detectors in the tube (Sum
option) or over time (Integrate). To switch between the option use the
context menu of the mini-plot:

.. figure:: /images/SumIntegrateMenu.png
   :alt: SumIntegrateMenu.png

| 

Summing over the detectors
--------------------------

With the Sum option the mini-plot displays a sum of the counts in all
detectors in a tube vs time of flight.

.. figure:: /images/MiniplotSum.png
   :alt: MiniplotSum.png

Integrating over the time of flight
-----------------------------------

With the Integrate option the mini-plot displays the counts integrated
over time of flight vs detector position in the tube.

| |MiniplotIntegrate.png|
| Detector positions can be shown as detector IDs, or distance form a
  tube's end, or the φ angle. Switch between the units using the
  mini-plot's context menu.

.. figure:: /images/DetectorPositionOptions.png
   :alt: DetectorPositionOptions.png

Navigate
========

The |PickTabZoomButton.png| tool button switches on the navigation mode
which is the same as in Render Tab.

Selecting Arbitrary Sets of Detectors
=====================================

The rest of the buttons in the top row are for making complex
selections. Buttons
|PickTabEllipseButton.png|\ |PickTabRectButton.png|\ |PickTabElRingButton.png|\ |PickTabRectRingButton.png|
are for drawing shapes, |PickTabEditButton.png| is for editing them.

Draw an ellipse
---------------

#. Click the |PickTabEllipseButton.png| button.
#. Click and hold the mouse button down to start drawing.
#. Drag to resize.

.. figure:: /images/DrawingEllipse.png
   :alt: DrawingEllipse.png
   :width: 300px

Edit a shape
------------

#. Switch on the editing tool |PickTabEditButton.png|.
#. Click on a shape you would like to edit. The selected shape displays
   control points as small white rectangles.
#. Drag the control points to resize the shape.
#. To translate the shape click inside its shaded area and drag.

To select multiple shapes draw a rubber band around them.

.. figure:: /images/SelectMuplipleShapes.png
   :alt: SelectMuplipleShapes.png
   :width: 300px

The selected shapes are indicated by drawing a bounding box around each
of them.

.. figure:: /images/SelectedMuplipleShapes.png
   :alt: SelectedMuplipleShapes.png
   :width: 300px

Only translation is possible for a multiple selection.

Sum selected detectors
----------------------

The mini-plot automatically sums the counts in the detectors covered by
the shapes and plots them vs time of flight.

.. raw:: mediawiki

   {{SlideNavigationLinks|MBC_Displaying_and_Navigating_Instrument|Mantid_Basic_Course|MBC_Masking_and_Grouping}}

.. |PickTabToolButtons.png| image:: /images/PickTabToolButtons.png
.. |PickTabZoomButton.png| image:: /images/PickTabZoomButton.png
.. |PickTabEditButton.png| image:: /images/PickTabEditButton.png
.. |PickTabEllipseButton.png| image:: /images/PickTabEllipseButton.png
.. |PickTabRectButton.png| image:: /images/PickTabRectButton.png
.. |PickTabElRingButton.png| image:: /images/PickTabElRingButton.png
.. |PickTabRectRingButton.png| image:: /images/PickTabRectRingButton.png
.. |PickTabPickPixelButton.png| image:: /images/PickTabPickPixelButton.png
.. |PickTabPickTubeButton.png| image:: /images/PickTabPickTubeButton.png
.. |PickTabAddPeakButton.png| image:: /images/PickTabAddPeakButton.png
.. |PickTabEraseButton.png| image:: /images/PickTabEraseButton.png
.. |PickSingleDetector.png| image:: /images/PickSingleDetector.png
.. |PickTube.png| image:: /images/PickTube.png
.. |MiniplotIntegrate.png| image:: /images/MiniplotIntegrate.png
