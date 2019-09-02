=====================
Mocking the Presenter
=====================

The view should be so simple that it does not require
testing. However, the presenter contains logic and should therefore be
tested. When testing the presenter we do not want to create the actual
GUI, instead we just want to ensure that the correct calls are made,
this is done via mocking see `unittest docs
<https://docs.python.org/3/library/unittest.mock-examples.html>`_ for
a detailed discussion. Here we will have a brief discussion of the
main points.

First are the import statements

.. code-block:: python

    from __future__ import (absolute_import, division, print_function)

    import sys
    import presenter
    import view

    import unittest
    from mantid.py3compat import mock

A different import is used for ``mock``, depending on whether we're
using Python 2 or 3.

The test class is then initialised:

.. code-block:: python

    class PresenterTest(unittest.TestCase):
        def setUp(self):
            self.view = mock.create_autospec(view.View)
        
            # mock view
            self.view.doSomethingSignal = mock.Mock()
            self.view.btn_click = mock.Mock()
            self.view.getValue = mock.Mock(return_value=3.14)
       
            self.presenter = presenter.Presenter(self.view)

``create_autospec`` mocks the class contained within the brackets. We
then need to explicitly mock the methods using ``mock.Mock``. In
addtion, when a return value is needed, this is provided in the call
to ``mock.Mock``.

A test is shown below:

.. code-block:: python

        def test_doSomething(self):
            self.presenter.handleButton()
            assert(self.view.getValue.call_count == 1)

We call the ``handleButton`` function and then use ``call_count`` to
ensure that the method from the view is called the correct number of
times. This is a more robust method for checking how many times a
function is called.

There is a ``assert_called_once`` function however this should be
avoided as it can easily lead to errors. This is because it is
expected that the function ``assert_called_twice`` exists but it does
not, however when you run the test it will always pass.

The last bit of code is to execute the tests:

.. code-block:: python

    if __name__ == "__main__":
        unittest.main()
