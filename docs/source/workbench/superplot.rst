.. _WorkbenchSuperplot:

=========
Superplot
=========

Superplot is a decorator widget of the plot window. It facilitates over-plotting
and manipulation of overplotted data. The superplot is accessible:

* directly in the plot window through the menu bar
* in the context menu of supported workspaces


**Widget description**

The superplot widget is composed of two distinct parts:

* left side of the plot window: a list of selected workspaces and spectra. This
  list contains workspaces names and under them the plotted data if any.
* bottom of the plot: a navigation bar that contains a slider and different
  buttons to navigate through the bins/spectra and control the plotted data.

.. image:: ../images/superplot_1.png
   :align: center


**Usage**

When the superplot is activated, its state is updated based on the current plot.
The list will contain the names of the selected workspaces and below each of
them, the plotted bins/spectra.

When one or many workspace(s) is(are) clicked in the list, the navigation bar
can be used to slide over the bins/spectra and update the plot accordingly. One
can then add the bin/spectrum currently pointed by the slider to the list by
pressing the hold button.

Each workspace and bin/spectrum is associated, in the list, with a delete
button. To remove a curve, one can also slide to the specific position and
release the hold button.
