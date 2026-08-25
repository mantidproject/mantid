.. _ISISReflectometryPlottingTab:

================================================
ISIS Reflectometry Plotting Tab Internals
================================================

This document describes the implementation of the Plotting tab in the
:ref:`ISIS Reflectometry Interface <ISISReflectometryInterface>`. It is intended
for developers changing the workspace tree, plot output types, plot controls, or
the connection to Mantid's matplotlib plotting functions.

The implementation follows Model-View-Presenter (MVP):

* the model owns plotting data and creates plot-ready workspaces;
* the view owns Qt widgets and translates between Qt and view-facing data;
* the presenter coordinates the model, view, batch interface, and plotting
  services.

The source is under
:code:`qt/scientific_interfaces/ISISReflectometry/GUI/Plotting`. Shared plot
request and rendering code is under the adjacent :code:`GUI/Common` directory.

Architecture
------------

The principal dependencies are:

.. code-block:: text

   BatchPresenter
       |
       v
   PlottingPresenter ----------------------> IPlottingView
       |                                         ^
       |                                         |
       +--> PlottingWorkspaceTree          QtPlottingView
       |                                         |
       +--> IPlottingModel                 QtPlottingWorkspaceTreeViewAdapter
       |
       +--> IPlotOptionsProvider
       |
       +--> IPlotter
       |
       +--> IActiveFigureMonitor

:code:`PlottingWorkspaceTree` is presenter-owned domain state rather than a Qt
tree model. The presenter converts this state into
:code:`PlottingWorkspaceTreeItemState` objects before passing it to the view.
This keeps output-type selection policy out of the model and Qt-specific tree
handling out of the presenter.

Model Files
-----------

The model directory contains data derived from completed reductions and the
operations that prepare selected data for plotting.

:code:`model/PlottingWorkspace.h`
#################################

Defines the domain types shared by the plotting model and presenter:

* :code:`PlottingWorkspaceTreeItemType` identifies reduction groups, runs, ADS
  workspace groups, and individual workspaces in the hierarchy.
* :code:`ReducedWorkspaceOutputType` identifies the reduced output represented
  by a workspace item, such as :code:`IvsQ`, :code:`IvsLambda`, or
  :code:`IvsQBinned`.
* :code:`PlottingWorkspaceTreeItem` is one node in the model-side hierarchy. It
  contains domain data only and has no selection or display state.
* :code:`PlottingWorkspace` contains the metadata needed to turn one reduced
  workspace into a requested plot output. This includes its ADS name, run
  numbers, containing workspace group, and period number.

:code:`model/IPlottingModel.h` and :code:`model/PlottingModel.h/.cpp`
################################################################################

:code:`IPlottingModel` is the presenter-facing interface for obtaining
plot-ready ADS workspace names. :code:`PlottingModel` implements it.

:code:`PlottingModel::workspacesForPlotting` dispatches according to the
selected :code:`PlotOutputType`:

* reflectivity curves use the selected reduced workspace names directly;
* spin asymmetry creates a cached workspace from the up and down members of a
  polarization workspace group;
* alignment creates a detector profile and optional fitted-peak workspaces from
  the corresponding raw time-of-flight workspace;
* detector maps extract detector spectra, optionally convert the x axis to
  wavelength, and create the selected numeric y axis.

Generated workspaces have private ISIS Reflectometry prefixes and are reused if
they already exist in the ADS. The model uses run and period metadata from
:code:`PlottingWorkspace` to find the matching raw workspace in :code:`TOF` or
:code:`__TOF`.

:code:`model/PlottingWorkspaceTree.h/.cpp`
############################################

Owns the workspace hierarchy and a name-to-:code:`PlottingWorkspace` lookup.
:code:`rebuild` derives both collections from a :code:`RunsTable`:

* only successful reduction groups and rows are considered;
* an output is added only when its named workspace still exists in the ADS;
* ADS workspace groups are expanded into child workspace items;
* run-number and period metadata is read from matrix workspaces;
* empty runs and groups are omitted.

:code:`items` exposes the hierarchy to the presenter.
:code:`plottingWorkspacesForNames` resolves leaf names selected in the view back
to their model metadata. It preserves the order supplied by the view and ignores
names that are not in the current tree.

Presenter Files
---------------

The presenter directory contains orchestration and all policy that determines
what the user may select or plot.

:code:`presenter/IPlottingPresenter.h`
######################################

Defines the interface used by :code:`BatchPresenter`. It receives the parent
batch presenter, processing state changes, instrument changes, and updated
:code:`RunsTable` data. This allows the Batch component to coordinate the tab
without depending on :code:`PlottingPresenter` directly.

:code:`presenter/PlottingPresenter.h/.cpp`
##########################################

Coordinates the complete tab workflow. Its responsibilities are to:

* subscribe to view notifications and active-figure changes;
* enable or disable output selection while reduction or autoreduction runs;
* rebuild the model-side workspace tree when the runs table changes;
* ask the state builder for output-specific tree and control state;
* resolve the user's selected workspace names;
* request plot-ready workspaces from :code:`IPlottingModel`;
* request rendering options from :code:`IPlotOptionsProvider`;
* warn before creating five or more plot items; and
* submit one or more :code:`PlotRequest` objects to :code:`IPlotter`.

An individual layout is submitted as one request per workspace. Overplot and
tiled layouts are submitted as one request containing all selected workspaces.

:code:`presenter/PlottingPresenterFactory.h`
################################################

Constructs an :code:`IPlottingPresenter` with the production plotter, options
provider, and plotting model. :code:`BatchPresenterFactory` uses this factory
when constructing a Batch tab.

:code:`presenter/PlotOutputTypeProperties.h/.cpp`
##################################################

Centralises behavior that varies by :code:`PlotOutputType`. Each output type
has a display name, accepted tree item types, accepted reduced output types, and
capability flags. The flags describe support for overplotting, adding to an
existing figure, postprocessed group outputs, and multi-plot selection based on
workspace groups.

The presenter and its state builder query these properties instead of
containing output-type conditionals. This is the main extension point for tree
selection and action behavior when adding another plot output type.

:code:`presenter/PlottingViewStateBuilder.h/.cpp`
#################################################

Builds all state passed from the presenter to the view. For the plotting
workspace tree, it converts model-side :code:`PlottingWorkspaceTreeItem`
objects into view-facing :code:`PlottingWorkspaceTreeItemState` objects for the
selected output type. It evaluates whether each node:

* is included by the output type;
* is a postprocessed group output that must be excluded;
* can be selected directly;
* can contribute when an ancestor is selected; and
* should be visually muted.

Keeping this policy in the presenter layer means that
:code:`PlottingWorkspaceTree` knows nothing about GUI selection behavior.

The same builder creates state for the other parts of the Plotting tab:

* output selector labels;
* visibility of detector-map and alignment controls; and
* enabled and checked states for individual, overplot, tiled, vertically tiled,
  and add-to-existing actions.

Action state depends on processing state, selected workspace and workspace-group
counts, output-type capabilities, and compatibility with the active
Reflectometry figure. When adding to an existing figure, one selected item is
enough for a tiled or overplot request because the figure already contains a
plot.

:code:`presenter/QtActiveFigureMonitor.h/.cpp`
################################################

:code:`IActiveFigureMonitor` abstracts notifications that the active matplotlib
figure may have changed. :code:`QtActiveFigureMonitor` implements this with a
Qt timer. The presenter uses the notification to refresh add-to-existing and
overplot action state when plots are opened, activated, or closed outside the
tab.

View Files
----------

The view directory contains Qt widgets and simple data transferred from the
presenter to the view.

:code:`view/IPlottingView.h`
############################

Defines:

* :code:`PlottingViewSubscriber`, the user-action notifications implemented by
  the presenter; and
* :code:`IPlottingView`, the state setters and user-input getters used by the
  presenter.

The interface uses standard C++ and plotting domain types. Qt is limited to the
plot parent :code:`QWidget` required by the plotting service.

:code:`view/PlottingViewState.h`
################################

Defines small view-state structures created by
:code:`PlottingViewStateBuilder`: :code:`PlotOutputTypeViewItem`,
:code:`PlotOutputControlsState`, and :code:`PlotActionState`. These structures
let the presenter update a related set of controls in one call.

:code:`view/PlottingWorkspaceTreeItemState.h`
#################################################

Defines the evaluated, view-facing workspace-tree node and its
:code:`PlottingWorkspaceTreeSelectionMode`. Unlike the model-side tree item,
this type contains muted and selection behavior that has already been decided
by the presenter layer.

:code:`view/QtPlottingView.h/.cpp`
##################################

Implements :code:`IPlottingView` as the Plotting tab widget. It:

* initialises controls defined in :code:`PlottingWidget.ui`;
* connects Qt signals to :code:`PlottingViewSubscriber` notifications;
* applies presenter-provided control state;
* reads the current output, axes, layout options, and selection;
* owns :code:`QtPlottingWorkspaceTreeViewAdapter`; and
* displays the confirmation dialog for large plot requests.

The view reports user actions but does not decide which actions or tree entries
are valid.

:code:`view/QtPlottingWorkspaceTreeViewAdapter.h/.cpp`
################################################################################

Adapts :code:`PlottingWorkspaceTreeItemState` objects to a
:code:`QStandardItemModel` displayed by
:code:`QtPlottingWorkspaceTreeView`. It owns the Qt-specific details of:

* columns and custom data roles;
* output and item type display names;
* palette-aware muted rendering;
* row and subtree selection propagation;
* direct-selection and parent-selection modes; and
* extracting selected leaf workspace names and selected workspace-group counts.

The adapter applies selection behavior already specified by the presenter. It
does not determine whether a particular output type permits a workspace.

:code:`view/QtPlottingWorkspaceTreeView.h/.cpp`
################################################

Provides the specialised :code:`QTreeView` used by the tab. Its painting logic
extends selected-row backgrounds across muted cells and obtains disabled colors
from the active Qt palette, allowing the tree to work in light and dark themes.

:code:`view/PlottingWidget.ui`
##############################

Qt Designer definition for the Plotting tab. It declares the workspace tree,
output selector, detector-map and alignment axis controls, plotting buttons,
vertical tiling checkbox, and add-to-existing checkbox. Control policy is not
encoded in the UI file; the presenter supplies it through
:code:`PlotActionState` and :code:`PlotOutputControlsState`.

Shared Plotting Files
---------------------

The following files under :code:`GUI/Common` are used by the Plotting tab and
keep plot request construction separate from Python-backed rendering.

:code:`Common/PlotOptions.h/.cpp`
#################################

Defines the plot output, layout, style, and axis enums together with three data
structures:

* :code:`PlotOutputSelection` is the user's scientific output and axis choices;
* :code:`PlotOptions` is the complete rendering configuration derived from that
  selection; and
* :code:`PlotRequest` combines workspace names, options, figure targeting, and
  window-parent information for :code:`IPlotter`.

The implementation provides output-specific factories for reflectivity,
detector-map, spin-asymmetry, and alignment options. These factories define
labels, scales, styles, error bars, markers, and window titles.

:code:`Common/IPlotOptionsProvider.h` and
:code:`Common/PlotOptionsProvider.h/.cpp`
################################################

:code:`IPlotOptionsProvider` is the presenter-facing interface.
:code:`PlotOptionsProvider` reports the output types available for an instrument
and converts a :code:`PlotOutputSelection` plus :code:`PlotLayout` into
:code:`PlotOptions`. Instrument-specific outputs are currently available for
POLREF, OFFSPEC, and CRISP; other instruments expose reflectivity curves only.

:code:`Common/IPlotter.h` and :code:`Common/Plotter.h/.cpp`
################################################################################

These files existed before the tab and were extended to support its requests.
:code:`IPlotter` is the presenter-facing rendering interface. It can report
whether the active figure belongs to the Reflectometry tab, whether that figure
can be overplotted, and plot a :code:`PlotRequest`.

:code:`Plotter` bridges C++ to Mantid's Python/matplotlib plotting functions. Its
main path has three stages:

#. Evaluate the request by expanding workspace groups, deriving matplotlib axis
   properties, finding an eligible active figure, and choosing a plotting route.
#. Dispatch to colorfill, existing-figure tiled, custom tiled, or standard
   Mantid plotting.
#. Apply post-plot processing to the appropriate axes: labels, optional
   horizontal markers, the Reflectometry figure marker, and transient window
   parenting.

The distinction between all axes and newly-created axes prevents labels and
markers for a tiled addition from modifying plots that were already in the
figure.

Integration and Build Files
---------------------------

:code:`Plotting/CMakeLists.txt`
###############################

Lists plotting sources, headers, the Qt MOC input, and
:code:`PlottingWidget.ui`, then exports those lists to the enclosing
ISISReflectometry CMake configuration.

:code:`Batch/BatchPresenter.h/.cpp` and
:code:`Batch/BatchPresenterFactory.h`
############################################

These existing Batch files integrate the new component. The factory constructs
the plotting presenter from the plotting view. :code:`BatchPresenter` owns it,
supplies the parent presenter, forwards instrument and processing state, and
calls :code:`updatePlottingWorkspaces` whenever reduction state or relevant ADS
state changes.

:code:`Batch/IBatchView.h` and :code:`Batch/QtBatchView.h/.cpp`
################################################################################

These existing view files expose and construct the :code:`QtPlottingView` as one
of the Batch tab's child tabs.

Test Files
----------

The focused test files are:

* :code:`test/Plotting/PlottingWorkspaceTreeTest.h`, covering hierarchy and
  metadata construction from runs-table and ADS state;
* :code:`test/Plotting/PlottingModelTest.h`, covering workspace selection and
  generation for each output type;
* :code:`test/Plotting/PlottingViewStateBuilderTest.h`, covering output labels,
  control visibility, and plot action enablement;
* :code:`test/Plotting/PlottingPresenterTest.h`, covering Batch and view
  notifications, display-state updates, request construction, and plotting
  orchestration;
* :code:`test/Plotting/QtPlottingViewTest.h`, covering Qt control and workspace
  tree behavior;
* :code:`test/Common/PlotOptionsProviderTest.h`, covering instrument output
  availability and output-to-options conversion; and
* :code:`test/Common/PlotterTest.h`, covering plot route selection and
  post-plotting behavior through the Python plotting boundary.

:code:`TestHelpers/PlottingTestHelpers.h` provides readable equality matchers
and formatted diagnostics for plotting workspace tree expectations. The test
files are registered in :code:`test/CMakeLists.txt`.

End-to-End Operation
--------------------

Construction
############

#. :code:`QtBatchView` constructs the Plotting tab view.
#. :code:`BatchPresenterFactory` asks :code:`PlottingPresenterFactory` to create
   the presenter for that view.
#. The plotting presenter subscribes to the view and starts the active-figure
   monitor.
#. :code:`BatchPresenter` takes ownership of the plotting presenter and passes
   itself as the parent coordinator.

Performing a reduction
######################

#. The user requests processing from the Runs tab. The Runs presenter forwards
   the request to :code:`BatchPresenter`, which coordinates the job manager and
   algorithm runner.
#. When a row or group algorithm completes,
   :code:`BatchPresenter::notifyAlgorithmComplete` updates reduction state in
   the job manager and refreshes the Runs view.
#. The Batch presenter calls :code:`updatePlottingWorkspaces`. The same refresh
   is made after an algorithm error, batch load, row or group edits, settings
   changes, workspace deletion or rename, and ADS clearing.
#. :code:`updatePlottingWorkspaces` passes the current model
   :code:`RunsTable` to :code:`PlottingPresenter::notifyRunsTableChanged`.

Updating the workspace tree
###########################

#. :code:`PlottingPresenter` asks :code:`PlottingWorkspaceTree` to rebuild from
   the runs table.
#. The tree walks reduction groups and rows that completed successfully and
   checks each recorded output against the ADS.
#. Existing ADS workspace groups are expanded. Each selectable leaf is recorded
   as a :code:`PlottingWorkspace` with run, containing-group, and period
   metadata.
#. The presenter reads the currently selected :code:`PlotOutputType` and asks
   :code:`PlottingViewStateBuilder::plottingWorkspaceTreeItemStates` to evaluate
   the hierarchy using :code:`PlotOutputTypeProperties`.
#. The resulting item states are passed to
   :code:`IPlottingView::setPlottingWorkspaceTreeItemStates`.
#. :code:`QtPlottingWorkspaceTreeViewAdapter` rebuilds its
   :code:`QStandardItemModel`, applies muted state and selection modes, and
   expands the tree.

User interaction
################

#. The selected instrument determines the entries in the plot-output selector
   through :code:`PlotOptionsProvider::availableTypes`.
#. Changing output type clears the current tree selection. The presenter then
   rebuilds the display state, shows the relevant axis controls, and recalculates
   enabled plotting actions.
#. Clicking a selectable tree row is handled by
   :code:`QtPlottingWorkspaceTreeViewAdapter`. Parent selection is propagated
   only to descendants whose presenter-supplied selection mode permits it.
#. A selection change notifies :code:`PlottingPresenter`. The presenter obtains
   selected leaf names and workspace-group counts from the view and asks
   :code:`PlottingViewStateBuilder` for a new :code:`PlotActionState`.
#. The active-figure monitor causes the same action-state calculation when the
   current matplotlib figure changes. Add-to-existing is enabled only for a
   compatible Reflectometry figure and output type. Overplotting additionally
   requires Mantid to report that the active axes are compatible.

Creating the plot
#################

#. Clicking Individual, Overplot, or Tiled sends the corresponding
   :code:`PlotLayout` notification to :code:`PlottingPresenter`.
#. The presenter resolves selected leaf names through
   :code:`PlottingWorkspaceTree::plottingWorkspacesForNames` and reads the
   current output and axis selections from the view.
#. :code:`PlottingModel::workspacesForPlotting` returns existing reduced
   workspace names or creates the derived workspaces required for the selected
   output.
#. :code:`PlotOptionsProvider::optionsFor` creates the axis, style, marker, error
   bar, and title configuration.
#. For a request of five or more plot items, the view asks the user to confirm.
#. The presenter creates :code:`PlotRequest` objects and calls
   :code:`IPlotter::plot`. Individual plots are sent separately; overplot and
   tiled outputs are sent together.
#. :code:`Plotter` evaluates and dispatches the request. Colorfill requests use
   Mantid's :code:`pcolormesh`; vertical or workspace-group tiling uses custom
   tiled axes; tiled additions create axes on the active Reflectometry figure;
   other line requests use the standard Mantid plotting function.
#. Post-plot processing labels the correct axes, adds any configured marker,
   marks the figure as owned by this interface, and assigns the Reflectometry
   window as its transient parent.
#. The presenter refreshes active-figure compatibility so the controls reflect
   the newly created or updated plot.

Adding a Plot Output Type
-------------------------

When extending the tab with another output type, update each responsibility at
its existing boundary:

#. Add the scientific selection type and any required controls to
   :code:`PlotOptions.h` and the view.
#. Add selection and action capabilities to
   :code:`PlotOutputTypeProperties.cpp`.
#. Add plot-ready workspace creation or selection to :code:`PlottingModel.cpp`.
#. Add rendering options and instrument availability to
   :code:`PlotOptions.cpp` and :code:`PlotOptionsProvider.cpp`.
#. Add focused model, presenter/state-builder, options-provider, view, and
   plotter tests for behavior introduced at each boundary.
