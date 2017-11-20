import types
from qtpy.QtWidgets import QApplication


def gui_test(test):
    """
    Decorator for GUI test methods. Creates a QApplication before
    executing the test.
    :param test: A test method.
    """
    def _wrapper(self):
        app = QApplication([''])
        test(self)

    return _wrapper


def meta_gui_test(name, bases, dic):
    """
    Converts a unittest.TestCase class to a GUI test case by wrapping all
    test methods in gui_test decorator. Usage:

        class MyWidgetTest(unittest.TestCase):

            __metaclass__ = meta_gui_test

            def test_something(self):
                ...

            def test_something_else(self):
                ...

    Which is equivalent to the definition:

        class MyWidgetTest(unittest.TestCase):

            @gui_test
            def test_something(self):
                ...

            @gui_test
            def test_something_else(self):
                ...

    :param name: Class name
    :param bases: Base classes
    :param dic: Class'e attributes
    """
    for name, attr in dic.items():
        if isinstance(attr, types.FunctionType) and name.startswith('test'):
            dic[name] = gui_test(attr)
    cls = type(name, bases, dic)
    return cls
