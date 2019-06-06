================
MVP Introduction
================

The MVP (Model, View, Presenter) pattern is a set of guidelines for
creating easy to maintain GUIs (graphical user interfaces). In
general:

- The View contains the 'look' of the GUI
- The Model does the 'hard sums' for the GUI (e.g. runs algorithms)
- The Presenter acts as a go between for the View and Model

It is important to note that the View should be simple (no logic) and
is usually not tested. The Model can be tested in a similar way to
other Python tests. The Model and View never communicate with each
other directly. The Presenter will extract relevant information from
the View and pass it to the Model. The Presenter may then pass the
result of the Model to the View. The Presenter will contain the GUI
logic and is tested using **mocking**.

These are guidelines and do not form a set of rules. It is sometimes
open to interpretation if some files are classed as a view or a model
(this will be discussed in detail later).

A more thorough explanation of MVP can be found at
:ref:`GuiDesignGuidelinesMVPIntro`.
