=====================
Mocking the Presenter
=====================

The view should be so simple that it does not require
testing. However, the presenter contains logic and should therefore be
tested. When testing the presenter we do not want to create the actual
GUI, instead we just want to ensure that the expected calls are made,
this is done via mocking see `unittest docs
<https://docs.python.org/3/library/unittest.mock-examples.html>`_ for
a detailed discussion. Here we will have a brief discussion of the
main points.

First are the import statements

.. code-block:: python

    import sys
    import presenter
    import view

    import unittest
    from unittest import mock


The test class is then initialised:

.. code-block:: python

    class PresenterTest(unittest.TestCase):
        def setUp(self):
            self.view = mock.create_autospec(view.View)

            # mock view
            self.view._button_clicked = mock.Mock()
            self.view.get_value = mock.Mock(return_value=3.14)

            self.presenter = presenter.Presenter(self.view)

``create_autospec`` mocks the class contained within the brackets. We
then need to explicitly mock the methods using ``mock.Mock``. In
addtion, when a return value is needed, this is provided in the call
to ``mock.Mock``.

A test is shown below:

.. code-block:: python

   def test_doSomething(self):
       self.presenter.handleButton()
       self.view.getValue.assert_called_once()

We call the ``handleButton`` function and then use ``assert_called_once``
to ensure that the method from the view is called the correct number of
times. This is a robust method for checking how many times a function is
called.

We could also use ``self.assertEqual(1, self.view.getValue.call_count)`` or
a python ``assert`` statement. However using more specific asserts from the
``mock`` and ``unittest`` libraries can make intent and error messages clearer.

The last bit of code is to execute the tests:

.. code-block:: python

    if __name__ == "__main__":
        unittest.main()
