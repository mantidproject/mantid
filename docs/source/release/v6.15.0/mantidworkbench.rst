========================
Mantid Workbench Changes
========================

.. contents:: Table of Contents
   :local:

Bugfixes
--------
- (`#40298 <https://github.com/mantidproject/mantid/pull/40298>`_) The :ref:`inelastic-moments` tab of the
  :ref:`interface-inelastic-data-processor` interface now moves the vertical sliders to the edge of the integration
  limits when new data is loaded for visualization.
- (`#40027 <https://github.com/mantidproject/mantid/pull/40027>`_) The :ref:`Filter_Events_Interface` no longer has a
  ``Split Sample Logs`` checkbox - since that option is deprecated and all logs will be split.
- (`#40473 <https://github.com/mantidproject/mantid/pull/40473>`_) :ref:`colorfill plots <Colorfill_Plots>` with log
  colorscale will no longer cause errors when the below keys are pressed.

  - `c`, `left`, `backspace`, `MouseButton.BACK` - The behaviour is similar to pressing "Back" button on the plot's toolbar.
  - `v`, `right`, `MouseButton.FORWARD` - The behaviour is similar to pressing "Forward" button on plot the toolbar.
  - `r`, `h`, `home` - The plot would be reset to Home view, i.e similar to pressing Home button of the plot

- (`#40577 <https://github.com/mantidproject/mantid/pull/40577>`_) Selecting multiple workspaces and right-clicking will
  now only display actions in the context menu that are compatible with every selected workspace.
- (`#40574 <https://github.com/mantidproject/mantid/pull/40574>`_) The :ref:`FitPropertyBrowser <WorkbenchPlotWindow_Fitting>`
  should now be guarded against an occasional IndexError when interacting with plots.
- (`#40491 <https://github.com/mantidproject/mantid/pull/40491>`_) Calling ``fig.savefig()`` immediately after
  ``fig.show()`` will no longer cause a deadlock in the Qt event loop. This has been fixed by ensuring that all queued
  GUI events are safely processed before moving on to the next operation.


InstrumentViewer
----------------

New features
############
- The new Instrument View has some new features:

  - (`#39949 <https://github.com/mantidproject/mantid/pull/39949>`_) Added the side-by-side projection. On instruments
    with no side-by-side panel positions defined in the IDF, this projection will try to optimise the arrangement of
    panels. For any detectors not in panels, they will appear in a grid, an abstract representation of the detectors.
  - (`#40410 <https://github.com/mantidproject/mantid/pull/40410>`_) New option to either maintain (or not) the aspect ratio
    of a 2D projection of an instrument. This can result in a more efficient use of screen space, and clearer projections.
  - (`#40508 <https://github.com/mantidproject/mantid/pull/40508>`_) One can now add and remove peaks. Peaks can either
    be added/removed individually, or all peaks on all selected detectors can be removed at once.
  - (`#40578 <https://github.com/mantidproject/mantid/pull/40578>`_) Added an option to show monitors.
  - (`#40623 <https://github.com/mantidproject/mantid/pull/40623>`_) Has a new Python interface to use the Instrument
    View in a Jupyter Notebook.
  - (`#40101 <https://github.com/mantidproject/mantid/pull/40101>`_) Added a peak overlay. Peaks workspaces can be
    selected within the interface and those peaks will be plotted on both the projection and the 1D plot.

  .. figure:: ../../images/6_15_release/new-iv-features.gif
   :class: screenshot
   :width: 987px


Bugfixes
############
- The new Instrument View also has some fixes:

  - (`#40415 <https://github.com/mantidproject/mantid/pull/40415>`_) Peak overlays in the now work when an instrument
    has multiple detector IDs per spectrum.
  - (`#40743 <https://github.com/mantidproject/mantid/pull/40743>`_) An error on closing no longer occurs due to a
    missing presenter.
  - (`#40198 <https://github.com/mantidproject/mantid/pull/40198>`_) When deleting lots of peaks in a short space of
    time, a crash should no longer occur when erasing peaks.


SliceViewer
-----------

New features
############
- (`#40055 <https://github.com/mantidproject/mantid/pull/40055>`_) :ref:`sliceviewer` now supports the ``Asinh``
  colormap, similar to ``log`` but valid for zero values.

Bugfixes
############
- (`#40477 <https://github.com/mantidproject/mantid/pull/40477>`_) :ref:`sliceviewer` now corrects for dot-pattern
  artifacts when rendering sparse data in log scale.

:ref:`Release 6.15.0 <v6.15.0>`
