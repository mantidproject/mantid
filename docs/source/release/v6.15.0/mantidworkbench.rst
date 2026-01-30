========================
Mantid Workbench Changes
========================

.. contents:: Table of Contents
   :local:

New Features
------------
- (`#40101 <https://github.com/mantidproject/mantid/pull/40101>`_) Added peak overlay to the new Instrument View. Peaks workspaces can be selected within the interface and those peaks will be plotted on both the projection and the 1D plot.


Bugfixes
--------
- (`#40298 <https://github.com/mantidproject/mantid/pull/40298>`_) The vertical sliders in the ``Moments`` tab of the :ref:`interface-inelastic-data-processor` are moved to the edge to the integration limits when new data is loaded for visualization.
- (`#40027 <https://github.com/mantidproject/mantid/pull/40027>`_) Removed `Split Sample Logs` checkbox from the `Filter Events` interface, since that option is deprecated, and all logs will be split.
- (`#40473 <https://github.com/mantidproject/mantid/pull/40473>`_) Fixed a bug in :ref:`colorfill plots <Colorfill_Plots>` with log colorscale where it was giving errors when the below keys are pressed. The behaviour of the colorfill plot after the fix is given below.
- (`#40473 <https://github.com/mantidproject/mantid/pull/40473>`_) `c`, `left`, `backspace`, `MouseButton.BACK` - The behaviour is similar to pressing Back button on plot the toolbar
- (`#40473 <https://github.com/mantidproject/mantid/pull/40473>`_) `v`, `right`, `MouseButton.FORWARD` - The behaviour is similar to pressing Forward button on plot the toolbar
- (`#40473 <https://github.com/mantidproject/mantid/pull/40473>`_) `r`, `h`, `home` - The plot would be reset to Home view, i.e similar to pressing Home button of the plot
- (`#40577 <https://github.com/mantidproject/mantid/pull/40577>`_) Fixed a bug where selecting multiple workspaces and right-clicking would open a context menu appropriate for one, but not necessarily all, of the workspaces. This could lead to errors (such as trying to plot a TableWorkspaces). The context menu now only displays actions that are compatible to all selected workspaces.
- (`#40574 <https://github.com/mantidproject/mantid/pull/40574>`_) Added an additional guard clause to fix the bug that causes an IndexError while interacting with the plots in the ``FitPropertyBrowser`` widget.
- (`#40491 <https://github.com/mantidproject/mantid/pull/40491>`_) Calling ``fig.savefig()`` immediately after ``fig.show()`` will no longer cause a deadlock in Qt event loop. This has been fixed by ensuring that all queued GUI events are safely processed before moving on to the next operation.


InstrumentViewer
----------------

New features
############
- (`#39949 <https://github.com/mantidproject/mantid/pull/39949>`_) Added the side-by-side projection to the new Instrument View. On instruments with no side-by-side panel positions defined in the IDF, this projection will try to optimise the arrangement of panels. For any detectors not in panels, they will appear in a grid, an abstract representation of the detectors.
- (`#40410 <https://github.com/mantidproject/mantid/pull/40410>`_) New option to either maintain (or not) the aspect ratio of a 2D projection of an instrument. This can result in a more efficient use of screen space, and clearer projections.
- (`#40508 <https://github.com/mantidproject/mantid/pull/40508>`_) In the new Instrument View, one can now add and remove peaks. Peaks can either be added/removed individually, or all peaks on all selected detectors can be removed at once.
- (`#40578 <https://github.com/mantidproject/mantid/pull/40578>`_) In the new Instrument View interface, add an option to show monitors.
- (`#40623 <https://github.com/mantidproject/mantid/pull/40623>`_) New Python interface to use the new Instrument View in a Jupyter Notebook

Bugfixes
############
- (`#40415 <https://github.com/mantidproject/mantid/pull/40415>`_) Peak overlays in the new Instrument View now work when an instrument has multiple detector IDs per spectrum.
- (`#40743 <https://github.com/mantidproject/mantid/pull/40743>`_) Fixed error on closing the new Instrument View due to a missing presenter.
- (`#40198 <https://github.com/mantidproject/mantid/pull/40198>`_) Fixed crash that could occur when erasing peaks in the Instrument View, especially when deleting lots of peaks in a short space of time.


SliceViewer
-----------

New features
############
- (`#40055 <https://github.com/mantidproject/mantid/pull/40055>`_) SliceViewer now supports "Asinh" colormap, similar to log but valid for zero values.

Bugfixes
############
- (`#40477 <https://github.com/mantidproject/mantid/pull/40477>`_) correct for dot-pattern artifact when rendering sparse data in log scale.

:ref:`Release 6.15.0 <v6.15.0>`
