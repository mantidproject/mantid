# ISIS Energy Transfer Tab

The ISIS Energy Transfer tab uses the MVP pattern for its
implementation. The code has undergone a refactoring process from a
single class structure to an MVP pattern, aimed at enhancing modularity,
testability, readability, and clear separation of concerns. The user
interface (UI) components are divided into three distinct classes: a
view, a model, and a presenter. This documentation will provide insights
into the design choices made for each class and suggest potential areas
for future improvements. The [Gui Standards](GuiStandards) documentation is used as a
guideline for writing the code.

## ISIS Energy Transfer Data

[Isisenergy Transfer Data](ISISEnergyTransferData) contains immutable data classes and constants
that facilitate communication among the view, presenter, and model
components for the ISIS Energy Transfer tab. The classes only provide
getters to prevent data modification, ensuring the stability and
integrity of the communication process.

## ISIS Energy Transfer view

The ISIS Energy Transfer View [Isisenergy Transfer View](ISISEnergyTransferView) follows the
[Gui Standards](GuiStandards) by being a simplistic component with minimal business
logic. Its functions define the interface for interacting with UI
elements. Key functionalities done by the view include:

### UI Data Getters

These functions are used to fetch data from the view. For example,
[Get Run Data](getRunData) returns [Ietrun Data](IETRunData) object that contains the values of
algorithm parameters. These functions should be constant and their
responsibility is get the values of the UI elements.

### UI data setters

The responsibility of these functions is to edit the values of different
UI elements. Ideally, these functions perform value assignment only,
avoiding any logical operations. An example is setDetailedBalance, which
assigns the detailed balance value in the UI.

### UI validators

Data validation ideally should be done in the model. However, there are
several cases where the validation cannot be done in the model.
Functions like [Is Run Files Valid](isRunFilesValid), [Validate Calibration File Type](validateCalibrationFileType), and
[Validate Rebin String](validateRebinString) are used to validate for such purposes.

### Communication with the Presenter

Communication between the view and presenter follows an observer pattern
to ensure decoupling from QT dependencies. The observer pattern
implementation relies on the IETViewSubscriber interface, serving as a
common subscriber interface. The view maintains a subscriber, typically
the presenter. In future iterations, multiple subscribers might be
notified concurrently. Subscribed functions are invoked through view
slots. For instance, the slot [Pb Run Finished](pbRunFinished) triggers the
[Notify Run Finished](notifyRunFinished) function in the subscriber.

### Future improvements

- Where feasible, shift validation functions to the model.
- Refactor [Set Default Instrument](setDefaultInstrument) and [Include Extra Grouping Option](includeExtraGroupingOption)
  because there are doing some logic.

## ISIS Energy Transfer Presenter

The ISIS Energy Transfer Presenter `IETPresenter` orchestrates
communication between the view and model. This class implements the
[Ietview Subscriber](IETViewSubscriber) interface to manage events coming from the view. The
presenter constructs both the view and model. The presenter undertakes
the following responsibilities:

### ISIS Energy Transfer algorithm

The presenter is involved in executing the core algorithm of the tab
[Isisindirect Energy Transfer](ISISIndirectEnergyTransfer). Functions such as `validate` ensure the
algorithm parameters are valid. [Notify Run Clicked](notifyRunClicked) serves as the event
handler for algorithm execution when a run click event happens. In
addition, `algorithmComplete` does post-algorithm processing operations.

### Plotting

Similar to the algorithm execution, the presenter manages data plotting.
The function [Notify Plot Raw Clicked](notifyPlotRawClicked) responds to the plot button click,
while [Plot Raw Complete](plotRawComplete) manages post-plotting tasks.

### Dependency on [Indirect Data Reduction Tab](IndirectDataReductionTab)

Currently, `IETPresenter` implements [Indirect Data Reduction Tab](IndirectDataReductionTab) which
does some UI logic, additionally coupled to QT. The presenter should not
have a dependency on QT but the current presenter is using this
interface to adapt with the legacy code. This is the first tab to be
refactored in the window. After refactoring all the tabs, the common
interface [Indirect Data Reduction Tab](IndirectDataReductionTab) should be refactored.

### Future improvements

- UI validation in `validate` could be moved to the model where
  possible.
- Add unit tests for the presenter

## ISIS Energy Transfer Model

The model is the place where the logic should be implemented. It defines
the interface to work with algorithms and other operations. The model
doesn't have a reference to the view and should be independent of the UI
framework. In the current implementation, `IETMdoel` is the model of the
tab.

### ISIS Indirect Energy Transfer Wrapper

[Isisindirect Energy Transfer](ISISIndirectEnergyTransfer) is the main algorithm in the tab. Various
functions manage operations related to algorithm execution. A series of
setters configure algorithm parameters. [Validate Run Data](validateRunData) employs
[Ietdata Validator](IETDataValidator) to validate algorithm parameters. Execution of the
algorithm takes place within `runIETAlgorithm` post validation and
parameters configuration.

### Plotting

In addition, the second responsibility for the model is to plot the
data. [Validate Plot Data](validatePlotData) uses [Ietdata Validator](IETDataValidator) to validate the
parameters of the plotting. The plotting occurs in [Plot Raw File](plotRawFile) which
validates and then plots the data.

### Saving

Saving the files is also handled in the model. The `saveWorkspace`
function calls different save operation depending on the file format
type (e.g. Nexus file).

### Grouping

Grouping is also done in the model. `groupWorkspaces` groups the
workspace based on the selected type of grouping.

### Model utils files

The [Isisenergy Transfer Model Utils](ISISEnergyTransferModelUtils) file contains several utility
functions assisting the model, including loading sample logs and
constructing grouping strings.

### Model tests

Model unit tests are defined in [Isisenergy Transfer Model Tests](ISISEnergyTransferModelTests).
Currently, the unit tests cover many functions in the model but ideally
it should cover all of the functions.

### Future improvements

- [Plot Raw File](plotRawFile) should be refactored. Currently, it is a big functions
  that run a lot of algorithms.
- Add unit tests for [Plot Raw File](plotRawFile), <span class="title-ref">save</span>,
  and `groupWorkspaces` functions.
