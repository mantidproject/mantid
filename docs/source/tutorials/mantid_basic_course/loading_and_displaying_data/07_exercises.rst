.. _07_exercises:

=========
Exercises 
=========

Part 1
======

#. Load the File "GEM38370_Focussed_Legacy.nxs".
#. Plot spectra 2-7 (all of them).
#. Edit the d-spacing axis range, or zoom into the range of 0-10
   angstroms.
#. Try changing the X-Axis to log scaling.

Part 2
======

#. Load the SANSLOQCan2D.nxs data. This is the output of a 2D SANS data
   reduction. Although a Workspace2D is mainly designed to store
   spectra, this is just the default, and in this data the loaded axes
   are momentum transfer Qx and the scattering cross section, and the
   'spectrum' axis is momentum transfer Qy.
#. Plot this data as a colour fill plot to get the following result:

.. figure:: /images/Sans2Dcolourfillplot_exercise1MBC.PNG
   :alt: centre
   :width: 600px

#. Change the colour bar axis to logarithm.
#. Change the Colour Map to Jet.
#. Create Contour Lines and labels in White at 20, 30 and 40.
#. Zoom in to see the results of your work.

Part 3
======

#. Using the workspaces MAR11015 and MAR11060 try to reproduce the plot
   below.

   -  You will need to group the two workspaces together and then
      right-click on the group to plot spectra 2-3.
   -  Adjust the axes to use log(x), linear(y) scaling

.. figure:: /images/MultiLayerGraph.png
   :alt: centre
   :width: 600px

.. raw:: mediawiki

   {{SlideNavigationLinks|MBC_Displaying_data_Formatting|Mantid_Basic_Course|MBC_Algorithms_History_EventWorkspace}}

