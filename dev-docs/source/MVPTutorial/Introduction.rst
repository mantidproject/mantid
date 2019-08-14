================
MVP Introduction
================

The MVP (Model, View, Presenter) pattern is a set of guidelines for
creating easy to maintain GUIs (graphical user interfaces). In
general:

- The view contains the 'look' of the GUI
- The model does the 'hard sums' for the GUI (e.g. runs algorithms)
- The presenter acts as a go between for the view and model

It is important to note that the view should be simple (no logic) and
is usually not tested. The model can be tested in a similar way to
other Python tests. The model and view never communicate with each
other directly. The presenter will extract relevant information from
the view and pass it to the model. The presenter may then pass the
result of the model to the view. The presenter will contain the GUI
logic and is tested using **mocking**.

These are guidelines and do not form a set of rules. It is sometimes
open to interpretation if some files are classed as a view or a model
(this will be discussed in detail later).

A more thorough explanation of MVP can be found at
:ref:`GuiDesignGuidelinesMVPIntro`.
