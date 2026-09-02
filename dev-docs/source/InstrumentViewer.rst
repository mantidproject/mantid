.. _InstrumentViewer:

===============
Instrument View
===============

.. contents:: Table of Contents
    :local:

Overview
--------

The Instrument View draws the geometry of an instrument, coloured by the data recorded in the
corresponding workspace, and lets the user select detectors, plot their spectra, overlay peaks and
build masks, regions of interest and detector groupings.

It is a pure-Python package built on `PyVista <https://docs.pyvista.org>`_ and VTK, and is
completely separate from the OpenGL widget described in :ref:`LegacyInstrumentViewer`. The two ship
side by side: ``Show Instrument`` opens the legacy widget, ``(Experimental) Show Instrument`` opens
this one.

The user documentation is at :ref:`mantid:InstrumentViewer`.

Where the code lives
--------------------

The package is ``qt/python/instrumentview``, and is registered from ``qt/python/CMakeLists.txt``
with:

.. code-block:: cmake

   add_python_package(instrumentview)
   add_dependencies(instrumentview mantidqt)

Workbench depends on it, through ``install_requires`` in
``qt/applications/workbench/setup.py`` and ``add_dependencies(workbench ... instrumentview)``.
There is no separate conda recipe; the package is installed as part of the ``mantidworkbench``
build, and its runtime dependencies ``pyvista``, ``pyvistaqt`` and ``superqt`` are pinned in
``conda/recipes/conda_build_config.yaml``.

.. list-table::
    :header-rows: 1
    :widths: 40 60

    * - Module
      - Responsibility
    * - ``FullInstrumentViewWindow.py``
      - ``FullInstrumentViewWindow`` (the ``QMainWindow`` shell) and
        ``FullInstrumentViewView`` (all of the widgets)
    * - ``FullInstrumentViewPresenter.py``
      - Event handling, renderer and interactor selection
    * - ``FullInstrumentViewModel.py``
      - All workspace-side logic; no Qt
    * - ``renderers/``
      - Building the detector meshes and the picking callbacks
    * - ``Projections/``
      - 2D projections of the detector positions
    * - ``InteractorStyles.py``
      - VTK mouse interaction styles
    * - ``ShapeWidgets.py``, ``ShapeOverlayManager.py``
      - The ROI and mask selection shapes, and the chart overlay they are drawn on
    * - ``ComponentTreeModel.py``, ``ComponentTreeView.py``, ``ComponentTreePresenter.py``
      - The instrument component tree
    * - ``Peaks/``
      - Reading and grouping peaks from a ``PeaksWorkspace``
    * - ``InstrumentViewADSObserver.py``
      - Reacting to workspaces being added, renamed or deleted
    * - ``NotebookView.py``, ``NotebookPresenter.py``, ``NotebookUtils.py``
      - A cut-down view for Jupyter notebooks
    * - ``InstrumentView.py``, ``__main__.py``
      - Standalone and command line entry points
    * - ``alfview/``, ``isisreflectometry/``
      - Specialisations embedded in other interfaces

Architecture
------------

The interface is Model/View/Presenter, as described in :ref:`MVPDesign`::

    FullInstrumentViewWindow (QMainWindow)
      └── FullInstrumentViewView ──────────── FullInstrumentViewPresenter ──── FullInstrumentViewModel
             │  widgets, plotter, line plot      on_* handlers, threading         workspace, detectors,
             │                                                                    masks, peaks
             ├── ShapeOverlayManager                     │
             │      └── SelectionShape                   ├── InstrumentRenderer
             ├── ComponentTreeView                       │      ├── PointCloudRenderer
             │      └── ComponentTreePresenter           │      ├── ShapeRenderer
             │             └── ComponentTreeModel        │      └── SideBySideShapeRenderer
             └── BackgroundPlotter (pyvistaqt)           ├── InteractorStyles
                                                         ├── Projection
                                                         └── InstrumentViewADSObserver

Note that ``FullInstrumentViewWindow.py`` contains two classes. ``FullInstrumentViewWindow`` is a
thin ``QMainWindow`` that sizes and centres itself and holds ``FullInstrumentViewView`` as its
central widget; all of the interesting view code is in the latter.

The three parts are assembled by the caller rather than by a factory, identically in
``InstrumentView.py`` and in Workbench
(``qt/applications/workbench/workbench/plugins/workspacewidget.py``):

.. code-block:: python

   model = FullInstrumentViewModel(ws)
   window = FullInstrumentViewWindow()
   FullInstrumentViewPresenter(window.get_instrument_view_widget(), model)

Connecting the view to the presenter
####################################

There are no custom Qt signals. Wiring happens in two steps from
``FullInstrumentViewPresenter.setup()``:

#. ``view.subscribe_presenter(self)`` stores the presenter on the view, which then *pulls* the
   contents of its combo boxes from it (``available_unit_options()``,
   ``count_scale_combo_options()``, ``peaks_workspaces_in_ads()``).
#. ``view.setup_connections_to_presenter()`` connects the stock Qt widget signals directly to
   presenter methods:

   .. code-block:: python

      self._projection_combo_box.currentIndexChanged.connect(self._presenter.on_projection_option_changed)
      self._mask_list.itemChanged.connect(partial(self._presenter.on_list_item_selected, CurrentTab.Masking))

The presenter never touches a widget directly; it calls named methods on the view such as
``set_contour_range_limits`` or ``show_plot_for_detectors``. The model imports no Qt at all.

Threading
---------

This is the part most likely to catch out a new developer. There is no ``QThread`` and no progress
bar. Responsiveness comes from three mechanisms working together.

**A worker thread in the presenter.** The presenter owns a daemon thread and a ``Queue``. Almost
every ``on_*`` handler does nothing but enqueue its ``_on_*`` counterpart, so the Qt thread returns
immediately:

.. code-block:: python

   self._callback_queue = Queue()
   self._callback_thread = Thread(None, self._callback_worker, daemon=True)
   self._callback_thread.start()

``handle_close()`` pushes a sentinel object to stop the worker.

**Automatic marshalling back onto the Qt thread.** ``FullInstrumentViewView`` is decorated with
``@run_on_qapp_thread()`` from ``mantidqt.utils.qt.qappthreadcall``, which replaces every *public*
method with a blocking ``QAppThreadCall``. The worker thread can therefore call view methods
directly. Note that private, underscore-prefixed methods are **not** wrapped, so a public method
that delegates to a private one is safe, but calling a private method from the worker thread is
not.

**VTK work must happen on the Qt thread first.** Anything that touches the VTK render window has to
run before the work is queued. ``on_shape_changed`` is the pattern to copy:

.. code-block:: python

   def on_shape_changed(self):
       centres = self._model.transformed_detector_positions
       self._view.project_and_cache_detector_points(centres)   # VTK, so Qt thread
       self._shape_update_generation += 1
       self._callback_queue.put((self._on_shape_changed, (centres, self._shape_update_generation)))

The generation counter lets the worker discard superseded updates, which matters when a shape is
dragged or the wheel is scrolled and events arrive faster than they can be processed.

Related helpers are the ``SuppressRendering`` context manager, used to batch plotter updates, and
the ``_skip_if_closing`` decorator, which guards view methods against calls arriving after
``closeEvent``.

Rendering
---------

The plotter is a ``pyvistaqt.BackgroundPlotter`` whose ``app_window`` is placed in a splitter
alongside a matplotlib canvas for the line plot:

.. code-block:: python

   self.main_plotter = BackgroundPlotter(show=False, menu_bar=False, toolbar=False, off_screen=self._off_screen)

``renderers/base_renderer.py`` defines the ``InstrumentRenderer`` abstract base class. It builds
three meshes -- the visible detectors, a pickable overlay used for highlighting the selection, and
the masked detectors -- and supplies the picking callback. The three implementations correspond to
the render modes offered in the GUI:

``PointCloudRenderer``
    One point per detector in a single ``pv.PolyData``, drawn as screen-space spheres. Picking uses
    a ``vtkPointPicker``.

``ShapeRenderer``
    Draws real detector geometry. Detectors are grouped by unique shape using
    ``componentInfo.shapeToComponentIndices()``, so the geometry is only built once per distinct
    shape. For each shape it either extracts an optimised four-vertex quad from the ``ShapeInfo``,
    or falls back to the full triangulated mesh from ``CSGObject.getMesh()``. The instances are
    then scaled, rotated and translated with vectorised NumPy using ``detectorInfo.allRotations()``,
    ``allScaleFactors()`` and ``allPositions()``, and merged into a **single** ``pv.PolyData`` so
    that VTK issues one draw call for the whole instrument. A ``_cell_to_detector`` array maps VTK
    cell IDs back to detector indices, both for picking and for writing the scalars.

``SideBySideShapeRenderer``
    Subclasses ``ShapeRenderer``. It uses nearest-neighbour distances per bank
    (``scipy.spatial.cKDTree``) to scale the shapes so that unrolled panels do not overlap, and
    applies bank rotations only to tube banks.

Switching render mode does not rebuild everything: the presenter keeps the renderer instances alive
and only reloads them when the workspace itself changes.

Projections
-----------

``Projections/Projection.py`` is a base class with a self-registering subclass registry, so a new
projection needs no changes to any factory or combo box:

.. code-block:: python

   class Projection:
       _registry = {}

       def __init_subclass__(cls, projection_types=None, **kwargs):
           for projection_type, defaults in projection_types.items():
               Projection._registry[projection_type] = (cls, defaults)

       def __new__(cls, type, **kwargs):
           if cls is Projection:
               subclass, _ = Projection._registry.get(type)
               return super().__new__(subclass)

Subclasses are imported at the bottom of the module so that they register themselves without
creating a circular import. ``ProjectionType`` is a ``str`` enum, and the GUI's list of projections
is derived from it directly.

- ``SphericalProjection`` uses ``u = -atan2(y, x)`` and ``v = -acos(v/r)``.
- ``CylindricalProjection`` is equal-area: ``u = -atan2(y, x)`` and ``v = z/|r|``.
- ``SideBySide`` finds the flat banks -- grid banks through ``instrument.findGridDetectors()`` and
  tube banks through the shared C++ ``PanelsSurfaceCalculator`` -- unrolls each into its own panel,
  and tiles the panels. Bank positions from the ``side-by-side-view-location`` tag in the IDF are
  honoured where present.

The base class also corrects the seam: it finds the largest gap in the ``u`` coordinate and shifts
the range by multiples of ``u_period`` so that a bank is not split across the wrap-around point.
``project_points()`` is the public entry point, and is what ``ShapeRenderer`` uses to project the
vertices of the detector shapes rather than just their centres.

Projections are cached on the model per ``(projection type, flip beam)`` pair. Fitting the
projection to the window is *not* part of the projection: the presenter computes a scale matrix in
``_transform_mesh_to_fill_window()`` and assigns it to ``model.transform``.

Picking and mouse interaction
-----------------------------

Each renderer supplies ``get_callback_tied_to_detector_index(plotter, callback, hover)``, which
wraps a ``vtkPointPicker`` or ``vtkCellPicker`` and translates a hit into an index into the
*pickable* detectors. Only the pickable mesh has ``pickable=True``, so masked detectors, monitors
and the sample cannot be picked. This differs from the legacy widget, which rendered a hidden image
with detector indices encoded as colours and read the colour back.

``InteractorStyles.py`` holds five VTK styles, and
``FullInstrumentViewPresenter._update_interactor_style()`` chooses between them, in this order:

.. list-table::
    :header-rows: 1
    :widths: 45 55

    * - Condition
      - Style
    * - Not a 2D projection
      - ``TRACKBALL``
    * - A shape overlay is active
      - ``SCROLL_ZOOM_NO_PICKING``
    * - Hover pick is checked
      - ``SCROLL_ZOOM_WITH_HOVER``
    * - Rectangle zoom is toggled on
      - ``RUBBERBAND_ZOOM``
    * - Otherwise
      - ``SCROLL_ZOOM_WITH_PICKING``

``CursorZoomInteractorStyle`` implements zooming about the cursor in parallel projection: it caches
the world position under the cursor during mouse-move events and, on a wheel event, adjusts the
camera's ``parallel_scale`` and shifts the focal point so that point stays put. It caches a default
camera state, resets to it on right-click, and snaps back to it when zoomed out past it. After each
change it fires ``camera_changed_callback`` so that a screen-space shape overlay can re-evaluate
which detectors it covers.

``RubberBandZoomInteractorStyle`` replaces VTK's default left-button observers so that holding
:kbd:`Shift`, :kbd:`Ctrl` or :kbd:`Alt` picks a detector instead of starting a rubber band.

Shape overlays
--------------

``ShapeWidgets.py`` defines a ``SelectionShape`` base class and the circle, rectangle, ellipse,
annulus and hollow rectangle implementations. Shapes work in normalised ``[0, 1]`` viewport
coordinates and provide ``outline_xy``, ``fill_coords``, ``hit_test``, ``apply_resize_delta`` and
``indices_in_shape``. ``hit_test`` returns ``"inside"``, ``"edge"``, ``"inner_edge"`` or
``"handle"``, which is what decides between moving, resizing and rotating, and which cursor is
shown.

``ShapeOverlayManager`` draws the shape on a transparent ``pv.Chart2D`` laid over the render, adds
the VTK mouse observers, projects the 3D detector positions into chart coordinates, and returns the
boolean mask of the detectors the shape covers.

The model
---------

``FullInstrumentViewModel`` keeps full-length arrays for every detector and a set of boolean masks
over them. Nearly every public property is a slice of one of those arrays:

.. code-block:: python

   self._is_valid = (self._is_monitor != "yes") & (self._workspace_indices != -1)
   is_pickable = ~self._is_masked & self._is_valid & self._is_selected_in_tree

Understanding which mask a property is sliced by is usually enough to understand the property. Note
in particular that indices coming back from a picking callback are indices into the *pickable*
detectors, not into all detectors.

Bulk detector data is read with the :ref:`CreateDetectorTable <algm-CreateDetectorTable>` algorithm
rather than a Python loop over detectors, which is much faster for large instruments:

.. code-block:: python

   detector_info_table = CreateDetectorTable(
       self._workspace, IncludeDetectorPosition=True, OneRowPerDetectorID=True,
       StoreInADS=False, EnableLogging=False
   )

Integrated counts come from the C++ method
``MatrixWorkspace.getIntegratedCountsForWorkspaceIndices``, again to avoid a per-spectrum loop. The
line plot is produced with :ref:`ExtractSpectra <algm-ExtractSpectra>`, then
:ref:`ConvertUnits <algm-ConvertUnits>` if the plot units differ, then, when summing more than one
spectrum, :ref:`Rebin <algm-Rebin>` onto a common grid followed by
:ref:`SumSpectra <algm-SumSpectra>`. The rebin is necessary because ragged workspaces cannot be
summed directly.

Component tree
--------------

``ComponentTreeModel`` wraps ``workspace.componentInfo()``. ``ComponentTreePresenter`` builds a
``QStandardItemModel`` lazily: a node that has children is given a single child with the text
``##placeholder##``, which is replaced with the real children the first time the node is expanded.
Without this, building the tree for a large instrument took several seconds.

Selection is propagated as the union of ``componentsInSubtree()`` for each selected node. The model
converts those component indices to detector IDs and sets ``_is_selected_in_tree``, so unselected
detectors move into the masked mesh and become unpickable.

Reacting to the ADS
-------------------

``InstrumentViewADSObserver`` subclasses ``AnalysisDataServiceObserver`` and wraps each of its five
callbacks in a ``QAppThreadCall``. The presenter then queues the real work onto its worker thread:

- **delete** closes the window if it was the displayed workspace, otherwise refreshes the lists.
- **rename** re-points the model at the renamed workspace and rebuilds the component tree.
- **replace** re-renders, or refreshes the peaks, mask or grouping list depending on the type.
- **add** adds new peaks, mask or grouping workspaces to the relevant list.
- **clear** closes the window.

``handle_close()`` deletes the observer explicitly rather than leaving it to the garbage collector,
which would otherwise keep stale references to workspaces alive.

Extending the Instrument View
-----------------------------

New render mode
    Implement ``InstrumentRenderer``, add the mode string to
    ``FullInstrumentViewView._RENDER_MODE_OPTIONS`` and a branch to
    ``FullInstrumentViewPresenter._get_renderer_for_mode``.

New projection
    Add a member to ``ProjectionType`` and subclass ``Projection`` with
    ``projection_types={ProjectionType.MY_TYPE: {...}}``. The combo box is populated from the enum,
    so nothing else needs changing.

New selection shape
    Subclass ``SelectionShape`` and add it to ``FullInstrumentViewView._shape_options``.

Reusing the view
    Subclass ``FullInstrumentViewView`` and override ``_set_layouts`` to build a different layout
    from the same widgets, overriding any ``set_*`` methods that do not apply. This is what
    ``alfview`` does.

Reusing only the model
    Compose ``FullInstrumentViewModel`` with a renderer and interactor styles directly, without
    ``FullInstrumentViewPresenter``. This is what ``isisreflectometry`` does.

.. warning::

   The values of ``Globals.CurrentTab`` are the literal tab label strings, and
   ``get_current_selected_tab()`` converts the label back with ``CurrentTab(tab_name)``. Renaming a
   tab in the GUI without changing the enum will break grouping and masking silently.

Interfaces that embed the Instrument View
#########################################

Both current consumers are C++ interfaces that import the Python presenter, and both are gated on
the QSettings flag ``InstrumentView/use_new_instrument_view``, exposed in Workbench under
``Settings`` -> ``General``.

``alfview/``
    ``ALFInstrumentViewPresenter`` and ``ALFInstrumentViewView`` subclass the full presenter and
    view. The view replaces the whole layout with a rebin box, a hover-pick button and a rectangle
    tool, and the presenter forces bank/tube selection, the ``Cylindrical Y`` projection and the
    approximated-shapes render mode. It calls back into C++ by finding a child ``QObject`` named
    ``ALFPythonCallbackRelay`` and invoking its ``notify`` method. Loaded by
    ``qt/scientific_interfaces/Direct/ALFPythonInstrumentView.cpp``.

``isisreflectometry/``
    ``ReflectometryInstrumentViewView`` is *not* a subclass; it is a minimal widget holding only a
    plotter, composed with ``FullInstrumentViewModel`` and ``ShapeRenderer``. Its plotter is created
    lazily in ``initialise()`` to avoid OpenGL context errors before the widget is embedded, and
    resize-driven updates are debounced with a timer. It relays selection changes through a child
    ``QObject`` named ``ShapeChangedRelay``. Loaded by
    ``qt/scientific_interfaces/ISISReflectometry/GUI/Preview/PreviewPythonInstrumentView.cpp``.

Testing
-------

Tests live inside the package, in ``test`` subdirectories next to the code they cover. They are
plain ``unittest`` test cases:

- Model tests use real workspaces from ``CreateSampleWorkspace(StoreInADS=False)``.
- Presenter tests use a ``MagicMock`` view. Where a mock stands in for the model, spec it against
  the real class so that a call to a method the model does not have fails the test.
- View tests use ``@start_qapplication`` from ``mantidqt.utils.qt.testing``, patch
  ``BackgroundPlotter`` and the matplotlib canvas, and patch ``force_method_calls_to_qapp_thread``
  so that the ``@run_on_qapp_thread`` decorator does not interfere.

.. warning::

   Test files are listed explicitly in ``PYTHON_TEST_FILES`` in
   ``qt/python/instrumentview/CMakeLists.txt``. A new test file will not run in CI until it is added
   there.

Run them with:

.. code-block:: sh

   ctest -R python.instrumentview

Differences from the legacy widget
----------------------------------

.. list-table::
    :header-rows: 1
    :widths: 20 40 40

    * - Concern
      - Legacy (C++)
      - Current (Python)
    * - Graphics
      - OpenGL, ``GLDisplay`` and ``GLObject``
      - VTK through PyVista
    * - Picking
      - Colour-encoded off-screen render
      - VTK pickers plus a cell-to-detector map
    * - Geometry
      - A geometry handler per object
      - Detectors grouped by unique shape into one merged mesh
    * - Projections
      - ``UnwrappedSurface`` and its subclasses
      - The ``Projections`` package
    * - Shapes
      - ``Shape2D`` and ``Shape2DCollection``
      - ``SelectionShape`` on a ``pv.Chart2D`` overlay
    * - Line plot
      - ``MiniPlotMpl``
      - Embedded matplotlib canvas with the Mantid toolbar
    * - Project save
      - ``InstrumentWidgetEncoder``
      - Not implemented
    * - Settings persistence
      - None
      - A few ``ConfigService`` keys
