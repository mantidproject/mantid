# ISIS Reflectometry Interface

This document gives a brief overview of the [ISIS Reflectometry
Interface](https://docs.mantidproject.org/nightly/interfaces/reflectometry/ISIS%20Reflectometry.html)
design and things that you should be aware of when working on this
interface. If you need to work on this interface, please make sure you
are familiar with the [Development guidelines](#development-guidelines)
below as a minimum.

## Overview

The [ISIS Reflectometry
Interface](https://docs.mantidproject.org/nightly/interfaces/reflectometry/ISIS%20Reflectometry.html)
provides a way for users to easily run a reduction on a *batch* of runs.
A batch of runs is entered into a table, which is actually a tree
structure with two levels - this allows sets of runs to be grouped so
that their outputs are post-processed (stitched) together. Various
default settings can be specified on the tabs. A tab is also provided to
make exporting the results for a set of workspaces easy.

The reduction for each row is done via
[Algm Reflectometry Isisload And Process](algm-ReflectometryISISLoadAndProcess) (which includes any
pre-processing). Post-processing for a group is done via
[Algm Stitch1dmany](algm-Stitch1DMany).

The GUI provides a lot of other functionality as well. Because it is
quite complex, it is important to keep to the established guidelines, in
particular sticking to the MVP design pattern, to avoid the code
becoming difficult to work with.

## Structure

### `GUI`

This directory contains all of the GUI code for the interface. Each
separate component e.g. the Experiment tab, has its own subdirectory.
Each of these components has its own view and presenter. There is also a
`Common` subdirectory for GUI components/interfaces common to more than
one widget (e.g. the [Irefl Message Handler](IReflMessageHandler) interface).

Briefly the structure is as follows:

- The top level is the [Main Window](MainWindow).
- This can have one or more vertical `Batch` tabs.
- Each `Batch` has several horizontal tabs:
  - The `Runs` tab is where the user specifies which runs to process.
    The actual runs list is specified in the embedded [Runs Table](RunsTable)
    component, which comprises of the generic
    [JobTreeView](../BatchWidget/API/JobTreeView) table along with a
    reflectometry-specific toolbar. The `Runs` tab also contains various
    other operations to do with finding and processing runs, such as
    searching and autoprocessing by investigation ID and a live data
    monitor. Note that a *table* here actually refers to a two-level
    tree, due to the way sets of runs can be grouped together for
    post-processing.
  - The `Experiment` tab allows the user to enter default settings
    related to a particular experiment.
  - The `Instrument` tab allows the user to enter default settings
    related to the current instrument.
  - The `Save` tab allows easy saving of a batch of outputs in ASCII
    format. It essentially just works on the ADS so this might not be
    necessary longer term if similar batch-saving functionality could be
    provided from the workspaces list.

<figure>
<img src="images/ISISReflectometryInterface_structure.png"
class="align-center"
alt="images/ISISReflectometryInterface_structure.png" />
</figure>

### `Reduction`

This directory contains models which centre around the *reduction
configuration*. This is a representation of all of the runs and settings
that have been entered in a particular Batch tab in the GUI. The top
level `Batch` model therefore provides everything needed to perform a
reduction for a particular set of runs.

Additionally, these models also contain state information, e.g. the
`Row` and `Group` contain information about whether processing has been
performed and what the output workspaces are.

### `Common`

This directory contains non-GUI-specific utility files useful in more
than one component of the reflectometry interface but that are still
specific to this interface, e.g. `Parse.h` contains parsing utility
functions that are specific for parsing reflectometry input strings such
as lists of run numbers. More generic utilities should be put elsewhere,
e.g. generic string handling functions might go in `Framework/Kernel`.

### [Test Helpers](TestHelpers)

This directory contains components specific for unit testing. The actual
tests are in `../test/ISISReflectometry/`.

## Reduction back-end

The back-end is primarily a set of algorithms, with the entry points
from the GUI being [Algm Reflectometry Isisload And Process](algm-ReflectometryISISLoadAndProcess) (for reducing
a row) and [Algm Stitch1dmany](algm-Stitch1DMany) (for post-processing a group). Any
additional processing should be added to these algorithms, or a new
wrapper algorithm could be added if appropriate (this might be necessary
in future if post-processing will involve more than just stitching).

The [Batch Presenter](BatchPresenter) is the main coordinator for executing a reduction.
It uses the [Batch Job Manager](BatchJobManager), which converts the reduction
configuration to a format appropriate for the algorithms. The conversion
functions are in files called [Row Processing Algorithm](RowProcessingAlgorithm) and
[Group Processing Algorithm](GroupProcessingAlgorithm), and any algorithm-specific code should be
kept to these files.

Unfortunately the whole batch cannot be farmed off to a single algorithm
because we need to update the GUI after each row completes, and we must
be able to interrupt processing so that we can cancel a large batch
operation. We also need to know whether rows completed successfully
before we can set up the group post-processing algorithms. Some queue
management is therefore done by the [Batch Presenter](BatchPresenter), with the help of
the [Batch Algorithm Runner](BatchAlgorithmRunner).

## Development guidelines

The following design principles should be adhered to when developing the
GUI. If the current design does not seem appropriate for additional
feature requests, do consult with a senior developer to work out the
best way forward rather than proceeding in a non-optimal way.

### Adhere to MVP

To ensure the GUI can be easily tested we follow the MVP design pattern.
There is general guidance on this [here](MVPDesign).

The view cannot easily be tested, so the aim of MVP is to keep the view
as simple as possible so that testing it is not necessary. Typically any
user action on the view results in a notification to the presenter and
is handled from there (even if that is just an update back to the view).
Even simple things like which buttons are enabled on startup are
controlled via the presenter rather than setting defaults in the view
itself.

It can be tempting to add one line to toggle or update something in the
view without wiring up the presenter. But these quick fixes can quickly
introduce bugs as they accumulate. The first question to ask yourself
before making any change is: how will I unit test it? In fact, we
recommend you follow [test driven
development](https://en.wikipedia.org/wiki/Test-driven_development) and
write the unit tests first.

Note that the views should not have a direct pointer to their
presenters, so the notification is done via a subscriber interface (see
[Subscriber pattern](#subscriber-pattern) for an example). The only
exception is the [Qt Main Window View](QtMainWindowView) (see [Dependency
inversion](#dependency-inversion)), but notifications should still be
done via the subscriber interface. This helps to avoid accidentally
introducing logic into the view about what should happen on an event and
instead just notify that an event happened. It could also be easily
extended to support multiple notifyees of different types, such as
different subscribed presenters.

### Dependency inversion

Dependency inversion has been introduced in an effort to simplify some
aspects of the design and to make the code more modular. Objects that a
class depends on are "injected", rather than being created directly
within the class that requires them. This makes testing easier, because
the real objects can easily be replaced with mocks. Most injection is
currently performed using constructors and takes place at the
'entry-point' for the Reflectometry GUI, in [Qt Main Window View](QtMainWindowView). See the
[Dependency injection](#dependency-injection) example below.

It is not normal in MVP for a view to have ownership of its presenter.
However since the whole of mantid does not use Dependency Injection, and
due to the way interfaces get instantiated this is currently necessary
for [Qt Main Window View](QtMainWindowView). This pointer should only be used for ownership
and all other usage should be avoided, so ensure you use the
[Main Window Subscriber](MainWindowSubscriber) interface to send notifications to the
presenter - i.e. use [M Notifyee](m_notifyee) instead of [M Presenter](m_presenter).

### Coordinate via presenters

Although the components are largely self-contained, there are occasions
where communication between them is required. For example, when
processing is running, we do not want the user to be able to edit any
settings, because this would change the model that the reduction is
running on. We therefore disable all inputs that would affect the
reduction when processing is running, and re-enable them when it stops.

Although enabling/disabling inputs in this example affects the views,
coordination between components is done via the presenters. This is to
ensure that all of these interactions can be unit tested. Each presenter
owns its child presenters, and also has a pointer to its parent
presenter (which is set by its parent calling [Accept Main Presenter](acceptMainPresenter) on
the child and passing a pointer to itself).

In the example mentioned, processing is initiated from e.g. the button
on the [Runs View](RunsView). This sends a notification to the [Runs Presenter](RunsPresenter) via
the subscriber interface. However, the [Runs Presenter](RunsPresenter) cannot initiate
processing because information is needed from the other tabs, and the
other tabs need to be updated after it starts. Processing therefore
needs to be coordinated at the Batch level. The [Runs Presenter](RunsPresenter)
therefore simply notifies its parent [Batch Presenter](BatchPresenter) that the user
requested to start processing. The [Batch Presenter](BatchPresenter) then does the work
to initiate processing. Once it has started (assuming it started
successfully) it then notifies all of its child presenters (including
the [Runs Presenter](RunsPresenter)) that processing is in progress.

Communication between different Batch components is also occasionally
required. For example, for usability reasons, only one autoprocessing
operation is allowed to be running at any one time. This means that when
autoprocessing is running, we need to disable the [Auto Process](AutoProcess) button
on all of the other Batch tabs as well. This must be coordinated via the
MainWindow component, which is the only component that has access to all
of the Batch tabs. The user initiates autoprocessing using the
[Auto Process](AutoProcess) button on the [Batch View](BatchView), which notifies the
[Batch Presenter](BatchPresenter) via the subscriber interface. Since the
[Batch Presenter](BatchPresenter) knows everything it needs to start autoprocessing for
that batch, it does the work to initiate autoprocessing itself. It then
simply notifies its parent [Main Window Presneter](MainWindowPresneter) that autoprocessing is
in progress (again, assuming that it started successfully). The
[Main Window Presenter](MainWindowPresenter) then notifies all of its child presenters that
autoprocessing is in progress so that they can enable/disable any
buttons/widgets as required.

### Avoid use of Qt types outside of Qt classes

Qt-specific types such as `QString`, `QColor` and subclasses of
`QWidget` should be kept out of the presenters and models. This avoids
confusion over which types should be used and a potentially messy
situation where we are always having to convert back and forth between
Qt types and `std` types. It also avoids an over-reliance on Qt, so that
the view could be swapped out in future to one using a different
framework, with little or no changes to the presenters and models.

To help make it clear where Qt is used, all classes that use Qt (namely
the views, along with a few supporting classes which wrap or subclass
`QObject`) are named with a `Qt` prefix in their file and class names.
Conversion from types like `QString` to `std::string` is performed
within the views, and no Qt types are present in their interfaces.

### Keep the reduction configuration up to date

Any change on the GUI's views results in a notification to the relevant
presenter, which typically then updates a relevant model in the
`Reduction` directory. The model should always be an up-to-date
representation of the view.

Model state (i.e. processed state for rows/groups and output workspace
names) should also be kept up to date. For example, if a row's output
workspace has been deleted, then its state is reset. If settings have
changed that would affect the reduction output, then the state is also
reset.

### Perform all processing in algorithms

When adding new functionality, where possible this should be done by
extending the algorithms rather than by adding logic to the GUI. The aim
is that there is a single algorithm that will be run for each entry in
the table (albeit a different algorithm for Rows and Groups).

Consider adding new wrapper algorithms if appropriate.
[Algm Reflectometry Isisload And Process](algm-ReflectometryISISLoadAndProcess) is an algorithm that has been
added specifically for this purpose and can usually be extended or
modified quite easily because it is designed for use with this GUI. The
post-processing algorithm, [Algm Stitch1dmany](algm-Stitch1DMany), is more generic so it is
likely in future that we would want to add a wrapper for this algorithm
rather than changing it directly.

## Design pattern examples

### Subscriber pattern

Let's take the `Event` component as an example.

- The view is constructed first and is passed to the presenter. The
  presenter then immediately subscribes to the view.

  ``` c++
  EventPresenter::EventPresenter(IEventView *view)
      : m_view(view) {
    m_view->subscribe(this);
  }
  ```

- This sets the notifyee in the view, using a subscriber interface.

  ``` c++
  void QtEventView::subscribe(EventViewSubscriber *notifyee) {
    m_notifyee = notifyee;
  }
  ```

- The subscriber interface defines the set of notifications that the
  view needs to send.

  ``` c++
  class MANTIDQT_ISISREFLECTOMETRY_DLL EventViewSubscriber {
  public:
    virtual void notifySliceTypeChanged(SliceType newSliceType) = 0;
    virtual void notifyUniformSliceCountChanged(int sliceCount) = 0;
    ...
  };
  ```

  Note that [Mantidqt Isisreflectometry Dll](MANTIDQT_ISISREFLECTOMETRY_DLL) is used to expose
  classes/functions so they can be used in different modules. In this
  case, it is needed in order for this class to be used in the tests,
  because the tests are not part of the ISISReflectometry library. If
  you get linker errors, this is one thing to check.

- The presenter implements the subscriber interface.

  ``` c++
  class MANTIDQT_ISISREFLECTOMETRY_DLL EventPresenter
      : public IEventPresenter,
        public EventViewSubscriber
  ```

- It overrides the notification functions to perform the relevant
  actions.

  ``` c++
  void EventPresenter::notifyUniformSliceCountChanged(int) {
    setUniformSlicingByNumberOfSlicesFromView();
    m_mainPresenter->notifySettingsChanged();
  }
  ```

- When a user interacts with the view, all the view needs to do is send
  the appropriate notification. By using an interface, the view does not
  know anything about the concrete type that it is notifying. This helps
  to avoid accidentally introducing logic into the view about what
  should happen on an event and instead just notify that an event
  happened. It could also be easily extended to support multiple
  notifyees of different types, such as different subscribed presenters.

  ``` c++
  void QtEventView::onUniformEvenChanged(int numberOfSlices) {
    m_notifyee->notifyUniformSliceCountChanged(numberOfSlices);
  }
  ```

### Dependency injection

A simple example of [Dependency inversion](#dependency-inversion) is in
the use of an [Irefl Message Handler](IReflMessageHandler) interface to provide a service to
display messages to the user. These messages must be displayed by a Qt
view. Rather than each view having to implement this, we use one object
(in this case the [Qt Main Window View](QtMainWindowView)) to implement this functionality
and inject it as an [Irefl Message Handler](IReflMessageHandler) to all of the presenters that
need it.

- The [Irefl Message Handler](IReflMessageHandler) interface defines the functions for
  displaying messages:

  ``` c++
  class IReflMessageHandler {
  public:
    virtual void giveUserCritical(const std::string &prompt,
                                  const std::string &title) = 0;
    ...
  };
  ```

- The [Qt Main Window View](QtMainWindowView) implements these:

  ``` c++
  void QtMainWindowView::giveUserCritical(const std::string &prompt,
                                          const std::string &title) {
    QMessageBox::critical(this, QString::fromStdString(title),
                          QString::fromStdString(prompt), QMessageBox::Ok,
                          QMessageBox::Ok);
  }
  ```

- The [Qt Main Window View](QtMainWindowView) creates a concrete instance of the interface
  (actually just a pointer to itself) and passes it in the construction
  of anything that needs it, e.g. the [Runs Presenter](RunsPresenter) (in this case
  using a factory to perform the construction - more about the [Factory
  pattern](#factory-pattern) below):

  ``` c++
  auto messageHandler = this;
  auto makeRunsPresenter = RunsPresenterFactory(..., messageHandler);
  ```

- The [Runs Presenter](RunsPresenter) then has a simple service it can use to display
  messages without needing to know anything about the
  [Qt Main Window View](QtMainWindowView):

  ``` c++
  m_messageHandler->giveUserCritical("Catalog login failed", "Error");
  ```

- Our unit tests can then ensure that a notification is sent to Qt in a
  known critical situation, e.g. in [Runs Presenter Test](RunsPresenterTest):

  ``` c++
  void testSearchCatalogLoginFails() {
    ...
    EXPECT_CALL(m_messageHandler,
                giveUserCritical("Catalog login failed", "Error"))
    .Times(1);
    ...
  }
  ```

### Factory pattern

The [Main Window Presenter](MainWindowPresenter) constructs the child Batch presenters on
demand. This prevents us injecting them in its constructor. In order to
follow [Dependency inversion](#dependency-inversion), we therefore need
to use factories to create the child presenters. Let's use the
[Main Window](MainWindow) -\> `Batch` -\> `Event` components as an example.

- As mentioned, the [Qt Main Window View](QtMainWindowView) is our entry point. This creates
  (and owns) the [Main Window Presenter](MainWindowPresenter). It:
  - creates an [Event Presenter Factory](EventPresenterFactory);
  - passes this to the [Batch Presenter Factory](BatchPresenterFactory) constructor so it can
    create the child [Event Presenter](EventPresenter) when needed;
  - passes this to the [Main Window Presenter](MainWindowPresenter) constructor, which accepts
    a [Batch Presenter Factory](BatchPresenterFactory), ready for making a Batch when needed.
- When required, we then create a Batch:
  - The [Qt Main Window View](QtMainWindowView) notifies the presenter that a new batch was
    requested.
  - The presenter instructs the view to create a child [Qt Batch View](QtBatchView)
    (which will also construct its child [Qt Event View](QtEventView)).
  - The [Qt Batch View](QtBatchView) is passed to the [Batch Presenter Factory](BatchPresenterFactory) to create
    the [Batch Presenter](BatchPresenter):
    - the [Batch Presenter Factory](BatchPresenterFactory) extracts the [Qt Event View](QtEventView) from the
      [Qt Batch View](QtBatchView);
    - this is passed to the [Event Presenter Factory](EventPresenterFactory) to create the child
      [Event Presenter](EventPresenter); it receives an [Ievent Presenter](IEventPresenter) back;
    - the batch factory injects the [Ievent Presenter](IEventPresenter) into the
      [Batch Presenter](BatchPresenter) constructor;
    - it returns the result as an [Ibatch Presenter](IBatchPresenter).
  - The [Ibatch Presenter](IBatchPresenter) is then added to the [Main Window Presenter](MainWindowPresenter)'s
    list of child presenters.

The [Main Window Presenter](MainWindowPresenter) therefore creates, and owns, the
[Batch Presenter](BatchPresenter), but does not need to know its concrete type. In turn,
the [Batch Presenter Factory](BatchPresenterFactory) creates the child [Event Presenter](EventPresenter) and
injects this into the [Batch Presenter](BatchPresenter), also without knowing the child's
concrete type. As mentioned in the [Dependency
inversion](#dependency-inversion) section, this helps testability by
allowing us to replace the real dependencies with mock objects.

## Testing

Let's look at the presenter-view interactions in the `Event` component
as an example.

- The [Event Presenter Test](EventPresenterTest) class sets up a mock view to use for testing:

  ``` c++
  NiceMock<MockEventView> m_view;
  EventPresenter makePresenter() {
    auto presenter = EventPresenter(&m_view);
    ...
    return presenter;
  }
  ```

- The mock view mocks any methods we're interested in testing, e.g. it
  mocks the subscribe method so that we can check that the presenter
  subscribes to the view:

  ``` c++
  class MockEventView : public IEventView {
  public:
    MOCK_METHOD1(subscribe, void(EventViewSubscriber *));
  ```

- The presenter then uses [Expect Call](EXPECT_CALL) to check that the method was
  called. Note that for `subscribe` it is difficult to check that the
  correct presenter pointer is passed because of the two-way dependency
  in the construction, so we just check that it is called with any
  argument; for other methods we typically want to check the exact
  arguments.

  ``` c++
  void testPresenterSubscribesToView() {
    EXPECT_CALL(m_view, subscribe(_)).Times(1);
    auto presenter = makePresenter();
    verifyAndClear();
  }
  ```

- We know that the only notifications we can get from the view are the
  set of methods in the subscriber interface. Our presenter tests should
  test each of these. We may also have functions in the presenter that
  are initiated from different callers than the view, e.g. the parent
  presenter, so we must test these too. Generally, we want to test all
  functions in the public interface to the presenter class - and
  exercise all possible code paths that can result.

- Note that it's likely we need multiple tests for each notification,
  for example [Notify Uniform Slice Count Changed](notifyUniformSliceCountChanged) has a test to check that
  the model is updated as well as one to check that the main presenter
  is notified:

  ``` c++
  void testChangingSliceCountUpdatesModel() {
    ...
    presenter.notifyUniformSliceCountChanged(expectedSliceCount);
    auto const &sliceValues =
        boost::get<UniformSlicingByNumberOfSlices>(presenter.slicing());
    TS_ASSERT(sliceValues ==
              UniformSlicingByNumberOfSlices(expectedSliceCount));
    verifyAndClear();
  }
  ```

  ``` c++
  void testChangingSliceCountNotifiesMainPresenter() {
    auto presenter = makePresenter();
    EXPECT_CALL(m_mainPresenter, notifySettingsChanged()).Times(AtLeast(1));
    presenter.notifyUniformSliceCountChanged(1);
    verifyAndClear();
  }
  ```

- Testing outcomes separately like this speeds up future development
  because it makes it easier to see where and why failures happen. It
  also makes it easier to maintain the tests as the code develops - e.g.
  if a functional change deliberately changes the expected action on the
  main presenter then we only need to update that test. The test that
  checks the model should not be affected (and if it is, we know we've
  broken something!).

- Note that although the [Event Presenter](EventPresenter) tests currently check the
  model directly, the model could (and should) be mocked out and tested
  separately if it was more complex.
