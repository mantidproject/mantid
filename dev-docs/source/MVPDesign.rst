.. _MVPDesign:

=====================
MVP Design
=====================

.. contents::
   :local:

Summary
#######

This page describes guidelines that should be followed when
implementing an interface in MantidWorkbench. The aim is to encourage a
consistent approach to developing interfaces.

.. _MVPDesignIntro:

MVP (Model View Presenter)
##########################

GUIs in Mantid aim to use the MVP pattern. The MVP pattern is a
generic concept for how to structure GUI code. MVP allows components
of the GUI to be tested separately and automatically. It also allows
for greater flexibility. Decoupling the model and view means that if
the developer wants to experiment with, for example, a different GUI
toolkit, or a different method of doing their calculations, then it is
easy and safe to swap out components. A description of each component
is given below.

To illustrate MVP, a simple example of a calculator GUI has been
created using Python (the concepts of MVP can be applied to any
programming language). This example can be found in
:ref:`MVPCalculatorGUIExample`, and you can run it with
``python Calculator.py``.

It is good practice to have model, view or presenter (as appropriate)
at the end of the name for each file (e.g. FFTView, FFTModel,
FFTPresenter), and each component should be a class in its own
right. Within the MVP pattern the model and view never exchange any
information directly.

Model
-----

The model is where the 'hard sums' take place within GUI. Any Mantid
algorithms should be run in the model, as well any other calculations
that may need to be performed.

It is possible that a presenter may have multiple models. For example
if two GUIs require the same calculation (e.g. mean) but not all
of the model (one GUI may need standard deviation and the other the
median), then it would be sensible for there to be three models (with
the mean model being shared). This prevents code duplication and makes
maintenance easier.

It is important to note that the values used in the calculations
should be received from the presenter (more of which below).

.. _MVPDesignView:

View
----

The view determines the look of the GUI. In passive-view MVP, there
will generally be very little logic in the view. A view should define
the following sections:

* The look of the GUI (often this will be defined in a Qt ``.ui`` file
  instead)
* **Get** methods to return values from the widgets (text input,
  checkbox etc)
* **Set** methods to update the output from the GUI (eg. plot some
  data, fill in some text boxes)

A view will probably also contain **connections**. A detailed
explanation of signals and slots can be foud `here
<http://doc.qt.io/archives/qt-4.8/signalsandslots.html>`_. Briefly, a
widget may emit **signals**. For example QPushButton emits the signal
``clicked`` when it is clicked. In order to handle the button being
clicked, the view will implement a **slot** method. This method does
whatever we need for a button click. To ensure that this method is
called whenever the button is clicked, we connect the ``clicked``
signal of our button to the ``handleButtonClick`` slot of our view.

The view should have a parent - this will be the widget containing
it. An example of a parent would be a main window containing tabs -
the children of the main window would be the tabs, and the children of
the tabs would be the widgets contained within the tabs.

Presenter
---------

The presenter acts as a 'go-between'. It receives data from the view,
passes it to the model for processing, receives it back from the model
and passes it to the view to be displayed to the user. The presenter
generally should contain relatively simple logic (though it will be
more complex than the view).

The model and the view are stored as members of the presenter
class. These should be passed into the presenter at initialisation.

It is important to note that the model and view should have as little
access as possible to the presenter. Presenter-model communication is
simple - the presenter generally just calls methods on the
presenter. Presenter-view communication is slightly more
involved. There are two ways of doing it:

* **Connections** - the presenter may contain connections as well as
  the view. You may choose to define custom signals in your view, such
  as a ``plotRequested`` signal to announce that the user has asked to
  plot some data, probably by clicking a button. The presenter will
  need to implement a slot (let's call it ``handlePlotRequested``) to
  handle this, which gets the relevant data from the model and passes
  it to the view. We then need to connect the signal to the slot in
  the presenter's constructor. It is also possible for a signal
  emitted by a view to be caught in the presenter of a parent view. In
  order to communicate by connections using Qt in C++ the presenter
  must inherit from ``QObject``. It's generally considered good
  practice to avoid having Qt in the presenter, so this method works
  best for GUIs written in Python (or another language with a more
  relaxed type system).

  - Note that is good practice to handle all signals in the presenter
    if you can, even if it is possible to just handle them in the
    view. This is because by going through the presenter we can unit
    test the handling of the signals.
* **Notify** - the presenter may instead allow the view to 'notify'
  it. This can be achieved by implementing a set of possible
  notifications (in C++ an enum class works well) and a method
  ``notify(notification)`` on the presenter. In the above example,
  ``handlePlotRequested`` is still needed, but now ``notify`` invokes
  it whenever it is passed a ``plotRequested`` notification. This
  method requires the view to have a pointer to the presenter, which
  introduces a circular dependency and leaks information about the
  presenter to the view. The leak can be resolved by having the
  presenter implement an interface which exposes **only** the
  ``notify`` method, and having the view keep a pointer to
  this.

Doing presenter-view communication with connections is the cleaner of
the two, so this method should be used unless writing a GUI in
C++. You'll notice that, in both cases, the view never passes data
(for example, the input from a text box) directly to the presenter,
instead it justs tells the presenter that something needs to be
done. In passive-view MVP the presenter, in handling this, gets any
data it needs from the view using the view's **get** methods.

Testing MVP Components
----------------------

MVP allows us to write automated tests for a large amount of the
GUI. We can write independent tests for the presenter and model, but
usually not the view (for this reason, the view should be as simple as
possible, ideally containing no logic at all).

**Mocking** is very useful tool for testing the presenter. Mocking
allows us to return a predefined result from a method of either the
view or the model.

It is useful to mock out the model because, providing that we've
written adequate tests for it, we don't care what the output is in the
tests for the presenter - we just care that the presenter handles it
correctly. The model may perform time-consuming calculations, such as
fitting, so by returning a dummy value from the fitting method we cut
down the time our tests take to run. We can also potentially change
how the model works - if the GUI uses an algorithm which undergoes
some changes, such as applying a different set of corrections, the
tests for the presenter will be unaffected.

It's useful to mock out the view because we don't want to have to
manually input data every time the unit tests are run - instead we can
mock the **get** methods to simulate the user entering data.

Using `GMock
<https://github.com/google/googletest/tree/main/googlemock>`_
in C++, or `unittest.mock
<https://docs.python.org/3/library/unittest.mock.html>`_ in Python, we
can set expectations in the unit tests for certain methods to be
called, and with certain arguments.

MVP Template tool
#################

The `template.py script <https://github.com/mantidproject/mantid/blob/main/tools/MVP/template.py>`__
provides a tool which is designed to generate initial files for an MVP-based
widget written in either Python or C++. These generated files serve as a
foundation or template for creating any widget using the MVP design pattern.
It can also be used to create the necessary files when refactoring an existing
widget which is not currently using MVP. The script is designed to be run from
within a `mantid-developer` Conda environment.

Python
------

To generate files for a Python widget with name "Example", run:

.. code-block:: sh

  python tools/MVP/template.py --name Example --language python --include-setup --output-dir $PWD/..


This command will generate five python files including `example_model.py`, `example_view.py`,
,`example_presenter.py` and `example_guiwidget.ui`. These files will be saved in the provided output directory,
as specified by `$PWD/..`. An additional file named `launch.py` will be generated if the
``--include-setup`` flag is provided to the script. This can be used to open the widget as
follows:

.. code-block:: sh

  python $PWD/../launch.py

C++
---

To generate files for a C++ widget with name "Example", run:

.. code-block:: sh

  python tools/MVP/template.py --name Example --language c++ --include-setup --output-dir $PWD/..

This command will generate nine files including `ExampleModel.cpp`, `ExampleModel.h`,
`ExampleView.cpp`, `ExampleView.h`, `ExamplePresenter.cpp`, `ExamplePresenter.h` and
`ExampleGUIWidget.ui`.
An additional file named `main.cpp` and a `CMakeLists.txt` will be generated if the
``--include-setup`` flag is provided to the script. These files can be used to build
the widget as follows:

.. code-block:: sh

  mkdir buildmvp
  cd buildmvp
  cmake ..
  cmake --build .

The example widget can then be opened with:

.. code-block:: sh

  cd buildmvp
  # On a Unix system
  ./launch
  # On a Windows system from a shell or bash
  ./Debug/launch.exe

The `main.cpp` and a `CMakeLists.txt` files are intended as an example for how you can
build, and then instantiate your widget. If you are refactoring or creating a new
widget for Mantid, the headers and cpp files should be included in the relevant
CMakeLists file elsewhere in the project.

Visual Design
#############

Qt Designer
-----------

The layout of all interfaces and reusable widgets should be done by
using the Qt's `Designer
<http://qt-project.org/doc/qt-4.8/designer-manual.html>`_ tool. This
has several advantages:

* immediate visual feedback of what the widget/interface will look
  like
* far easier to maintain, e.g. moving a control is a simple drag and
  drop
* reduces the amount of hand-written code required

If it is felt that the design must be hand coded then this should be
discussed with a senior developer.

Reusable Widgets
----------------

Many interfaces will require similar functionality. For example, the
ability to enter a filename string to search for a file along with a
'Browse' button to select a file from the filesystem. This type of
behaviour should be captured in a new composite widget that can be
reused by other components.

The new widget should be placed in the MantidWidgets plugin and a
wrapper created in the DesignerPlugins plugin so that the new widget
type can be used from within the Qt Designer.

The current set of reusable items are:

+-------------------------+---------------+--------------------------------------------------------------------------------------------------------------------------------------------------------------+
| Class Name              | Parent Class  | Abiltity                                                                                                                                                     |
+=========================+===============+==============================================================================================================================================================+
| AlgorithmSelectorWidget | QWidget       | A text box and tree widget to select an algorithm                                                                                                            |
+-------------------------+---------------+--------------------------------------------------------------------------------------------------------------------------------------------------------------+
| CatalogSearch           | QWidget       | An interface interface to the catalog system                                                                                                                 |
+-------------------------+---------------+--------------------------------------------------------------------------------------------------------------------------------------------------------------+
| CatalogSelector         | QWidget       | Displays the available catalog services                                                                                                                      |
+-------------------------+---------------+--------------------------------------------------------------------------------------------------------------------------------------------------------------+
| CheckBoxHeader          | QHeaderView   | Enables checkboxes to exist in the table header                                                                                                              |
+-------------------------+---------------+--------------------------------------------------------------------------------------------------------------------------------------------------------------+
| ColorBarWidget          | QWidget       | Show a color bar that can accompany a colored bidimensional plot                                                                                             |
+-------------------------+---------------+--------------------------------------------------------------------------------------------------------------------------------------------------------------+
| DataSelector            | MantidWidget  | A box to select if input is from a file or workspace along with the appropriate widget to choose a workspace or file.                                        |
+-------------------------+---------------+--------------------------------------------------------------------------------------------------------------------------------------------------------------+
| DisplayCurveFit         | MantidWidget  | A plot to display the results of a curve fitting process                                                                                                     |
+-------------------------+---------------+--------------------------------------------------------------------------------------------------------------------------------------------------------------+
| FindReplaceDialog       | QDialog       | A dialog box to find/replace text within a ScriptEditor                                                                                                      |
+-------------------------+---------------+--------------------------------------------------------------------------------------------------------------------------------------------------------------+
| FitPropertyBrowser      | QDockWidget   | Specialisation of QPropertyBrowser for defining fitting functions                                                                                            |
+-------------------------+---------------+--------------------------------------------------------------------------------------------------------------------------------------------------------------+
| FunctionBrowser         | QWidget       | Provides a wiget to alter the parameters of a function                                                                                                       |
+-------------------------+---------------+--------------------------------------------------------------------------------------------------------------------------------------------------------------+
| InstrumentSelector      | QCombobox     | A selection box populated with a list of instruments for the current facility                                                                                |
+-------------------------+---------------+--------------------------------------------------------------------------------------------------------------------------------------------------------------+
| LineEditWithClear       | QLineEdit     | A QLineEdit with a button to clear the text                                                                                                                  |
+-------------------------+---------------+--------------------------------------------------------------------------------------------------------------------------------------------------------------+
| MessageDisplay          | QWidget       | Display messages from the logging system                                                                                                                     |
+-------------------------+---------------+--------------------------------------------------------------------------------------------------------------------------------------------------------------+
| FileFinderWidget        | MantidWidget  | Provides a line edit to enter filenames and a browse button to browse the file system                                                                        |
+-------------------------+---------------+--------------------------------------------------------------------------------------------------------------------------------------------------------------+
| MWView                  | QWidget       | A colored, bidimensional plot of a matrix workspace                                                                                                          |
+-------------------------+---------------+--------------------------------------------------------------------------------------------------------------------------------------------------------------+
| ProcessingAlgoWidget    | QWidget       | A composite widget that allows a user to select if a processing step is achieved using an algorithm or a Python script. It also provides a script editor.    |
+-------------------------+---------------+--------------------------------------------------------------------------------------------------------------------------------------------------------------+
| ScriptEditor            | QsciScintilla | The main script editor widget behind the ScriptWindow                                                                                                        |
+-------------------------+---------------+--------------------------------------------------------------------------------------------------------------------------------------------------------------+
| WorkspaceSelector       | QComboBox     | A selection box showing the workspaces currently in Mantid. It can be restricted by type.                                                                    |
+-------------------------+---------------+--------------------------------------------------------------------------------------------------------------------------------------------------------------+

Icons
-----

Icons are a contentious subject as they can in some cases cause more
confusion and hinder more than they help. The NHS came up with a
good set of rules for what icons should be used and this could be
useful to designers, check out this `article. <https://digital.nhs.uk
/blog/transformation-blog/2019/icons-avoid-temptation-and-start-with
-user-needs>`_. It may fit a situation more to have a text button
instead of an icon.

Whilst having too many icons will confuse the average user there are
cases where many cases where it would help, for example if a button does a
similar thing to another button somewhere else in the program then
it should have the same icon. Have a look to see if the need you has
an icon in Mantid by look at this handy :ref:`MantidUsedIconsTable`.

Python
######

Interfaces can also be created in Python using the `qtpy
<https://pypi.org/project/QtPy/>`_ package. The code for the
interface should be placed in a Python `package
<https://docs.python.org/2/tutorial/modules.html#packages>`_ under the
``Code/Mantid/scripts`` directory. It should be named after the interface
name (without spaces). The code within the package should be
structured to avoid placing all of the code in a single file,
i.e. separate files for different classes etc. Sub packages are
recommended for grouping together logical sets of files.

For the interface to appear from within MantidWorkbench, create a startup
python file under the ``Code/Mantid/scripts`` directory. Assuming the code
for the interface is in a directory called foo_app then the startup
file would look like:

.. code-block:: python

   from foo_app import FooGUI

   app = FooGUI()
   app.show()

where ``FooGUI`` is the ``MainWindow`` for the interface. Some more
detailed documentation on creating GUIs in Python can be found at
:ref:`QtDesignerForPython`.


Designer
--------

As with the C++ GUI the Qt Designer should be used for layouts of all
widgets and the main interface. It is recommended that the ``.ui``
files be placed in a ``ui`` subdirectory of the interface package. To
generate PyQt code from the UI xml you will need to run the ``pyuic5``
program that ships with PyQt5. It is also recommended that the output
file is named, using the ``-o`` argument, ``ui_[widgetname].py`` and
placed in the ``ui`` subdirectory.
