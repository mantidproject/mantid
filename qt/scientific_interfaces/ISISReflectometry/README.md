# `ISIS Reflectometry`

This document gives a brief overview of the `ISIS Reflectometry` interface design and things that you should be aware of when working on this interface. If you need to work on this interface, please read the guidance at the end of this document.

## `Overview`

The `ISIS Reflectometry` interface provides a way for users to easily run a reduction on a "batch" of runs. A batch of runs is entered into a table, which is actually a tree structure with two levels - this allows sets of runs to be grouped so that their outputs are post-processed (stitched) together. Various default settings can be specified on the tabs. A tab is also provided to make saving a batch of outputs easy.

The reduction for each row is done via ReflectometryISISLoadAndProcess (which includes any pre-processing). Post-processing for a group is done via Stitch1DMany.

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

This contains models which centre around the "reduction configuration". This is a representation of all of the data that has been entered in the GUI. The top level `Batch` model provides everything needed to perform a reduction.

Additionally these models also contain some state information, e.g. the `Row` and `Group` contain information about whether processing has been performed

### `Common`

Contains non-GUI-specific utility files useful in more that one component of the reflectometry interface but that are still specific to this interface; more generic utilities should be put elsewhere, e.g. in Framework.

### `TestHelpers`

This file contains components specific for unit testing. The actual tests are in `../test/ISISReflectometry/`.

## `Reduction back-end`

The back-end is primarily a set of algorithms, with the entry points from the GUI being ReflectometryISISLoadAndProcess (for reducing a row) and Stitch1DMany (for post-processing a group). Any additional processing should be added to these algorithms, or a new wrapper algorithm could be added if appropriate (this might be necessary in future if post-processing will involve more than just stitching).

The `BatchPresenter` is the main coordinator for executing a reduction. The `BatchJobRunner` converts the reduction configuration to a format appropriate for the algorithms. The conversion functions are in `RowProcessingAlgorithm` and `GroupProcessingAlgorithm`, and any algorithm-specific code should be kept to these files.

Unfortunately the whole batch cannot be farmed off to a single algorithm because we need to update the GUI after each row completes, and we must be able to interrupt processing so that we can cancel a large batch operation. We also need to know whether rows completed successfully before we can set up the group post-processing algorithms. Some queue management is therefore done by the `BatchPresenter`, with the help of the `BatchAlgorithmRunner`.

## `Development Guidelines`

The following design principles should be adhered to when developing the GUI. If current design does not seem appropriate for additional feature requests, do consult with a senior developer to work out the best way forward rather than proceeding in a non-optimal way.

### Adhere to MVP

To ensure the GUI can be easily tested we follow the MVP design pattern. There is general guidance on this here: https://developer.mantidproject.org/GUIDesignGuidelines.html

The view cannot be tested so should be kept as simple as possible - typically any user action on the view results in a notification to the presenter and is handled from there (even if that is just an update back to the view).

The views should not have a direct pointer to their presenters, so the notification is done via a subscriber interface. The only exception is the QtMainWindowView, but notifications should still be done via the subscriber interface.

### Coordinate via presenters

Coordination between different widgets is done via the presenters. Each presenter owns any child presenters, and has a pointer to its parent presenter which is set by the parent calling `acceptMainPresenter` on the child.

Coordination between horizontal tabs is done by notifying up to the `BatchPresenter` which then notifies its child components. Coordination between different batch tabs is not usually required, but on the occasions it is (e.g. to ensure only one autoprocessing operation can run at a time) this is done via calls to the `MainWindowPresenter`.

### Avoid use of Qt types outside of Qt classes

Qt specific types such as QString, QColor and subclasses of QWidget should be kept out of the presenters and models. All classes that use Qt (namely the views, along with a few supporting classes which wrap or subclass QObjects) are named with a `Qt` prefix to make it clear where Qt is used. Conversion from types like QString to std::string is performed within the view, and no Qt types are present in their interfaces. 

### Keep the reduction configuration up to date

Any change on the GUI's views results in a notification to the relevant presenter, which typically then updates a relevant model in the `Reduction` directory. The model should always be an up-to-date representation of the view.

Model state (i.e. processed state for rows/groups and output workspace names) should also be kept up to date. For example, if a row's output workspace has been deleted, then its state is reset. If settings have changed that would affect the reduction output, then the state is also reset.

### Perform all processing in algorithms

When adding new functionality, consider whether it could be done by extending the algorithms rather than by adding logic to the GUI. The aim is that there is a single algorithm that will be run for each entry in the table so consider adding new wrapper algorithms if appropriate rather than calling multiple algorithms from the GUI.

### Dependency Inversion

Dependency inversion has been introduced in an effort to simplify some aspects of the design and to make the code more modular and easier to test. Most injection is currently performed using constructors and takes place at the 'entry-point' for the Reflectometry GUI, in QtMainWindowView.

It is not normal in MVP for a view to have ownership of its presenter. However since the whole of mantid does not use Dependency Injection, and due to the way interfaces get instantiated this is currently necessary for QtMainWindowView. Use of this for any other purpose should be avoided, so ensure you use the `MainWindowSubscriber` interface to send notifications to the presenter instead.
