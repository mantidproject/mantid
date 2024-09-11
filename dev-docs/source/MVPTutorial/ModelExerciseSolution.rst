.. _ModelExerciseSolution:

=======================
Model Exercise Solution
=======================

The model should now contain the following top level function:

.. code-block:: python

    def line_colours(self) -> List[str]:
        colour_table = ["red", "blue", "black"]
        return colour_table

The view should contain the following method:

.. code-block:: python

     def set_colours(self, options: List[str]) -> None:
         self._colours.clear()
         self._colours.addItems(options)

The presenter initialisation should now be:

.. code-block:: python

     def __init__(self, view, model):
        self._view = view
        self._model = model

        self._view.subscribe_presenter(self)

        self._view.set_colours(self._model.line_colours())

And the Main module should now pass the two models into the presenter:

.. code-block:: python

    import sys

    from qtpy.QtWidgets import QApplication

    from model import Model
    from view import View
    from presenter import Presenter


    def _get_qapplication_instance() -> QApplication:
        if app := QApplication.instance():
            return app
        return QApplication(sys.argv)


    app = _get_qapplication_instance()
    model = Model()
    view = View()
    presenter = Presenter(view, model)
    view.show()
    app.exec_()
