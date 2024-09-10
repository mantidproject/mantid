.. _CreatingThePresenter:

==========================
Creating a Presenter class
==========================

In the :ref:`Add Button <AddButton>` section we had the response to a button press
within the view. In practice this is not a good implementation. If the
response was more complicated then it would be difficult to maintain
the view as it would become extremely long. Furthermore creating the
look of the GUI is fairly simple and any logic/responses should be
contained within the presenter.

In this section we will make a simple presenter for when a button is
pressed.

The View
########

First we will start with the view:

.. code-block:: python

    from qtpy.QtWidgets import QHBoxLayout, QLabel, QPushButton, QVBoxLayout, QWidget
    from typing import Union


    class View(QWidget):

        def __init__(self, parent: Union[QWidget, None]=None):
            super().__init__(parent)
            self.setWindowTitle("view tutorial")

            # A presenter will be subscribed to the view later
            self._presenter = None

            self._button = QPushButton("Hi", self)
            self._button.setStyleSheet("background-color:lightgrey")
            # connect button to signal
            self._button.clicked.connect(self._button_clicked)

            self._label = QLabel()
            self._label.setText("Button")

            # add widgets to layout
            self._sub_layout = QHBoxLayout()
            self._sub_layout.addWidget(self._label)
            self._sub_layout.addWidget(self._button)

            grid = QVBoxLayout(self)
            grid.addLayout(self._sub_layout)
            # set the layout for the view widget
            self.setLayout(grid)

        def subscribe_presenter(self, presenter) -> None:
            # Subscribe the presenter to the view so we do not need to
            # make a Qt connection between the presenter and view
            self._presenter = presenter

        def _button_clicked(self) -> None:
            print("hello from view")
            self._presenter.handle_button_clicked()

The above code has two new additions. The first is a function which can
be used to subscribe the presenter to the view. The second addition is
that ``_button_clicked`` now calls a member function of the presenter to
handle the response to the button being clicked.

.. tip::

   Instead of connecting signals between the view and presenter, we have
   subscribed the presenter to the view.

   The key advantage of this approach is that the presenter does **not**
   need to be a `QObject`. This helps us follow a core principle of the
   MVP pattern: keeping Qt-specific code confined to the view for better
   separation of concerns.

   Why is this useful?

   - It makes the presenter easier to test and maintain.
   - The view and presenter are more decoupled, improving flexibility.
   - You can reuse the presenter in non-Qt environments.

   This compartmentalization keeps the codebase cleaner and more modular.

The Presenter
#############

The presenter is initialized with the view provided as a parameter. This
flexibility is a key advantage of the MVP pattern: the view can be swapped
out by passing a different one to the presenter. As long as the new view
adheres to the same interface as the previous one, the functionality remains
the same, but will have a different appearance.

A practical example of this is adapting the view based on device resolution.
You could switch to a different layout or design depending on the user's
screen size, ensuring an optimized experience across devices.

.. code-block:: python

    class Presenter:

        # pass the view into the presenter
        def __init__(self, view):
            self._view = view
            # subscribe the presenter to the view
            self._view.subscribe_presenter(self)

        # handle events from the view
        def handle_button_clicked(self) -> None:
            print("hello world, from the presenter")

Notice that the presenter is subscribed to the view in the constructor. This
is important if you want your callback from the view to work when a button is
clicked.

The Main
########

The main is now:

.. code-block:: python

    import sys

    from qtpy.QtWidgets import QApplication

    from view import View
    from presenter import Presenter


    def _get_qapplication_instance() -> QApplication:
        if app := QApplication.instance():
            return app
        return QApplication(sys.argv)


    app = _get_qapplication_instance()
    view = View()
    presenter = Presenter(view)
    view.show()
    app.exec_()

The view and presenter are both created, and the view is passed
into the presenter.
