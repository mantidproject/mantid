import types
from PyQt4.QtGui import QApplication
from PyQt4.QtCore import Qt


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


def gui_test_case(cls):
    """
    Converts a unittest.TestCase class to a GUI test case by wrapping all
    test methods in gui_test decorator. Usage:

        @gui_test_case
        class MyWidgetTest(unittest.TestCase):

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

    :param cls: Class instance
    """
    for name in dir(cls):
        attr = getattr(cls, name)
        if isinstance(attr, types.MethodType) and name.startswith('test'):
            setattr(cls, name, gui_test(attr))
    return cls


def select_item_in_tree(tree, item_label):
    """
    Select an item in a QTreeWidget with a given label.
    :param tree: A QTreeWidget
    :param item_label: A label text on the item to select.
    """
    items = tree.findItems(item_label, Qt.MatchExactly | Qt.MatchRecursive)
    tree.setCurrentItem(items[0])


def select_item_in_combo_box(combo_box, item_text):
    """
    Select an item in a QComboBox with a given text.
    :param combo_box: A QComboBox.
    :param item_text: A text of the item to select.
    """
    i = combo_box.findText(item_text)
    combo_box.setCurrentIndex(i)
