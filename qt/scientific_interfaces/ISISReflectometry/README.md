# `ISIS Reflectometry`

This document gives a brief overview of the `ISIS Reflectometry` interface design and things that you should be aware of when working on this interface. If you need to work on this interface, please read the guidance at the end of this document.

## `Overview`

The `ISIS Reflectometry` interface provides a way for users to easily run a reduction on a "batch" of runs. A batch of runs is entered into a table, which is actually a tree structure with two levels - this allows sets of runs to be grouped so that their outputs are post-processed (stitched) together. Various default settings can be specified on the tabs. A tab is also provided to make exporting the results for a set of workspaces easy.

The reduction for each row is done via `ReflectometryISISLoadAndProcess` (which includes any pre-processing). Post-processing for a group is done via `Stitch1DMany`.

The GUI provides a lot of other functionality as well. Because it is quite complex, it is important to keep to the established guidelines, in particular sticking to the MVP design pattern, to avoid the code becoming difficult to work with.

## `Directories`

### `GUI`

This contains all of the GUI code for the interface. Each separate component e.g. the Experiment tab, has its own subdirectory. Each of these components has its own view and presenter. There is also a `Common` subdirectory for GUI components/interfaces common to more than one widget.

Briefly the structure is as follows:

- The top level is the `MainWindow`.
- This can have one or more vertical `Batch` tabs.
- Each `Batch` has several horizontal tabs:
    - The `Runs` tab is where the user can search for runs, and specify the batch of runs to be processed via the `RunsTable` widget.
    - The `Experiment` tab allows the user to enter default settings related to a particular experiment.
    - The `Instrument` tab allows the user to enter default settings related to the current instrument.
    - The `Save` tab allows easy saving of a batch of outputs in ASCII format. This might not be necessary longer term if a similar batch-saving facility was provided e.g. from the workspaces list.

### `Reduction`

This contains models which centre around the "reduction configuration". This is a representation of all of the data that has been entered in the GUI. The top level `Batch` model provides everything needed to perform a reduction for a particular set of runs.

Additionally these models also contain some state information, e.g. the `Row` and `Group` contain information about whether processing has been performed and what the output workspaces are.

### `Common`

Contains non-GUI-specific utility files useful in more than one component of the reflectometry interface but that are still specific to this interface; more generic utilities should be put elsewhere, e.g. in Framework.

### `TestHelpers`

This directory contains components specific for unit testing. The actual tests are in `../test/ISISReflectometry/`.

## `Reduction back-end`

The back-end is primarily a set of algorithms, with the entry points from the GUI being `ReflectometryISISLoadAndProcess` (for reducing a row) and `Stitch1DMany` (for post-processing a group). Any additional processing should be added to these algorithms, or a new wrapper algorithm could be added if appropriate (this might be necessary in future if post-processing will involve more than just stitching).

The `BatchPresenter` is the main coordinator for executing a reduction. It uses the `BatchJobRunner`, which converts the reduction configuration to a format appropriate for the algorithms. The conversion functions are in `RowProcessingAlgorithm` and `GroupProcessingAlgorithm`, and any algorithm-specific code should be kept to these files.

Unfortunately the whole batch cannot be farmed off to a single algorithm because we need to update the GUI after each row completes, and we must be able to interrupt processing so that we can cancel a large batch operation. We also need to know whether rows completed successfully before we can set up the group post-processing algorithms. Some queue management is therefore done by the `BatchPresenter`, with the help of the `BatchAlgorithmRunner`.

## `Development guidelines`

The following design principles should be adhered to when developing the GUI. If the current design does not seem appropriate for additional feature requests, do consult with a senior developer to work out the best way forward rather than proceeding in a non-optimal way.

### Adhere to MVP

To ensure the GUI can be easily tested we follow the MVP design pattern. There is general guidance on this [here](https://developer.mantidproject.org/GUIDesignGuidelines.html).

The view cannot easily be tested, so the aim of MVP is to keep the view as simple as possible so that testing it is not necessary. Typically any user action on the view results in a notification to the presenter and is handled from there (even if that is just an update back to the view). Even simple things like which buttons are enabled on startup are controlled via the presenter rather than setting defaults in the view itself.

The views should not have a direct pointer to their presenters, so the notification is done via a subscriber interface (see the [Subscriber pattern](#subscriber-example) example below). The only exception is the `QtMainWindowView` (see [Dependency inversion](#dependency-inversion)), but notifications should still be done via the subscriber interface.

### Coordinate via presenters

Coordination between different widgets is done via the presenters. Each presenter owns any child presenters, and has a pointer to its parent presenter which is set by the parent calling `acceptMainPresenter` on the child.

Coordination between horizontal tabs is done by notifying up to the `BatchPresenter` which then notifies its child components. Coordination between different batch tabs is not usually required, but on the occasions it is (e.g. to ensure only one autoprocessing operation can run at a time) this is done via calls to the `MainWindowPresenter`.

### Avoid use of Qt types outside of Qt classes

Qt specific types such as `QString`, `QColor` and subclasses of `QWidget` should be kept out of the presenters and models. All classes that use Qt (namely the views, along with a few supporting classes which wrap or subclass `QObject`s) are named with a `Qt` prefix to make it clear where Qt is used. Conversion from types like `QString` to `std::string` is performed within the view, and no Qt types are present in their interfaces. 

### Keep the reduction configuration up to date

Any change on the GUI's views results in a notification to the relevant presenter, which typically then updates a relevant model in the `Reduction` directory. The model should always be an up-to-date representation of the view.

Model state (i.e. processed state for rows/groups and output workspace names) should also be kept up to date. For example, if a row's output workspace has been deleted, then its state is reset. If settings have changed that would affect the reduction output, then the state is also reset.

### Perform all processing in algorithms

When adding new functionality, consider whether it could be done by extending the algorithms rather than by adding logic to the GUI. The aim is that there is a single algorithm that will be run for each entry in the table so consider adding new wrapper algorithms if appropriate rather than calling multiple algorithms from the GUI.

### <a name="dependency-inversion"></a> Dependency inversion

Dependency inversion has been introduced in an effort to simplify some aspects of the design and to make the code more modular and easier to test. Most injection is currently performed using constructors and takes place at the 'entry-point' for the Reflectometry GUI, in `QtMainWindowView`. See an [example below](#dependency-injection-example).

It is not normal in MVP for a view to have ownership of its presenter. However since the whole of mantid does not use Dependency Injection, and due to the way interfaces get instantiated this is currently necessary for `QtMainWindowView`. Use of this for any other purpose should be avoided, so ensure you use the `MainWindowSubscriber` interface to send notifications to the presenter instead.

## Design pattern examples

### <a name="subscriber-example"></a> Subscriber pattern

Let's take the `Event` component as an example.

- The view is constructed first and is passed to the presenter. The presenter then immediately subscribes to the view.
``` cpp
EventPresenter::EventPresenter(IEventView *view)
    : m_view(view), m_sliceType(SliceType::None) {
  m_view->subscribe(this);
}
```
- This sets the notifyee in the view, using a subscriber interface.
``` cpp
void QtEventView::subscribe(EventViewSubscriber *notifyee) {
  m_notifyee = notifyee;
}
```
- The subscriber interface defines the set of notifications that the view needs to send.
``` cpp
class MANTIDQT_ISISREFLECTOMETRY_DLL EventViewSubscriber {
public:
  virtual void notifySliceTypeChanged(SliceType newSliceType) = 0;
  virtual void notifyUniformSliceCountChanged(int sliceCount) = 0;
  virtual void notifyUniformSecondsChanged(double sliceLengthInSeconds) = 0;
  virtual void
  notifyCustomSliceValuesChanged(std::string pythonListOfSliceTimes) = 0;
  virtual void
  notifyLogSliceBreakpointsChanged(std::string logValueBreakpoints) = 0;
  virtual void notifyLogBlockNameChanged(std::string blockName) = 0;
};
```
- The presenter implements the subscriber interface.
``` cpp
class MANTIDQT_ISISREFLECTOMETRY_DLL EventPresenter
    : public IEventPresenter,
      public EventViewSubscriber
```
- It overrides the notification functions to perform the relevant actions.
``` cpp
void EventPresenter::notifyUniformSliceCountChanged(int) {
  setUniformSlicingByNumberOfSlicesFromView();
  m_mainPresenter->notifySettingsChanged();
}
```
- When a user interacts with the view, all the view needs to do is send the appropriate notification. The view does not know anything about the concrete type that it is notifying. This helps to avoid accidentally introducing logic into the view about what should happen on an event and instead just notify that an event happened. It could also be easily extended to support multiple notifyees of different types. 
``` cpp
void QtEventView::onUniformEvenChanged(int numberOfSlices) {
  m_notifyee->notifyUniformSliceCountChanged(numberOfSlices);
}
```

### <a name="dependency-injection-example"></a> Dependency injection

A simple example of dependency injection is in the use of an `IMessageHandler` interface to provide a service to display error/warning messages to the user. These messages must be displayed by a Qt view. Rather than each view having to implement this, we use one object (in this case the `QtMainWindowView`) to implement this functionality and pass it as an `IMessageHandler` to all of the presenters that need it.

- The interface defines the options for displaying messages:
``` cpp
class IMessageHandler {
public:
  virtual void giveUserCritical(const std::string &prompt,
                                const std::string &title) = 0;
  ...
};
```
- The `QtMainWindowView` implements these:
``` cpp
void QtMainWindowView::giveUserCritical(const std::string &prompt,
                                        const std::string &title) {
  QMessageBox::critical(this, QString::fromStdString(title),
                        QString::fromStdString(prompt), QMessageBox::Ok,
                        QMessageBox::Ok);
}
```
- The `QtMainWindowView` sets up the concrete instance (actually just a pointer to itself) and passes it in the construction of anything that needs it, e.g. the `RunsPresenter` (in this case using a factory - more about factories [below](#factory-pattern)):
``` cpp
auto messageHandler = this;
auto makeRunsPresenter = RunsPresenterFactory(..., messageHandler);
```
- The `RunsPresenter` then has a simple service it can use to display messages without needing to know anything about the `QtMainWindowView`:
``` cpp
m_messageHandler->giveUserInfo("Search field is empty", "Search Issue");
```

### <a name="factory-pattern"></a> Dependency injection through constructors and factories

The `MainWindowPresenter` constructs the child Batch presenters on demand. This prevents us passing the child objects in the constructor. We solve this using factories to create the child presenters. Let's use the `MainWindow` -> `Batch` -> `Event` components as an example.

- As mentioned, the `QtMainWindowView` is our entry point. This creates (and owns) the `MainWindowPresenter`. It:
    - creates an `EventPresenterFactory`;
    - passes this to the `BatchPresenterFactory` constructor so it can create the child `EventPresenter` when needed;
    - passes this to the `MainWindowPresenter` constructor ready for making a Batch when needed.
- When required, we then create a Batch:
    - The `QtMainWindowView` notifies the presenter that a new batch was requested (via the `MainWindowSubscriber` interface).
    - The presenter instructs the view to create a child `QtBatchView` (which will also construct its child `QtEventView`).
    - The `QtBatchView` is passed to the `BatchPresenterFactory` to create the `BatchPresenter`:
        - the `BatchPresenterFactory` extracts the `QtEventView` from the `QtBatchView`;
        - this is passed to the `EventPresenterFactory` to create the child `EventPresenter`; it receives an `IEventPresenter` back;
        - the factory passes the `IEventPresenter` to the `BatchPresenter` constructor;
        - the factory returns this as an `IBatchPresenter`.
    - The `IBatchPresenter` is then added to the `MainWindowPresenter`'s list of child presenters.
    
The `MainWindowPresenter` therefore creates, and owns, the `BatchPresenter`, but does not need to know its concrete type. Similarly, the `BatchPresenter` creates, and owns, the `EventPresenter` but does not know its concrete type.

## Testing

Let's look at the presenter-view interactions in the `Event` component as an example.

- The `EventPresenterTest` class sets up a mock view to use for testing:
``` cpp
NiceMock<MockEventView> m_view;
EventPresenter makePresenter() {
  auto presenter = EventPresenter(&m_view);
  ...
  return presenter;
}
```
- The mock view mocks any methods we're interested in testing, e.g. it mocks the subscribe method so that we can check that the presenter subscribes to the view (although it is difficult to check that the correct presenter pointer is passed because of the two-way dependency in the construction, so we just check that it is called with any argument; for other functions we can typically check the exact arguments):
``` cpp
void testPresenterSubscribesToView() {
  EXPECT_CALL(m_view, subscribe(_)).Times(1);
  auto presenter = makePresenter();
  verifyAndClear();
}
```
- The only calls we can get from the view are the set of notifications in the subscriber interface. Our presenter tests should test each of these. Note that we may have multiple tests for each notification, for example `notifyUniformSliceCountChanged` has a test to check that the model is updated as well as this one to check that the main presenter is notified:
``` cpp
void testChangingSliceCountNotifiesMainPresenter() {
  auto presenter = makePresenter();
  EXPECT_CALL(m_mainPresenter, notifySettingsChanged()).Times(AtLeast(1));
  presenter.notifyUniformSliceCountChanged(1);
  verifyAndClear();
}
```

Note that we may have functions in the presenter that are initiated from different callers than the view, e.g. the parent presenter, so we must test these too. Generally, we want to test all functions in the public interface to the presenter class - and exercise all possible code paths that can result.

Also note that although the `EventPresenter` tests currently check the model directly, the model could (and should) be mocked out and tested separately if it was more complex.
