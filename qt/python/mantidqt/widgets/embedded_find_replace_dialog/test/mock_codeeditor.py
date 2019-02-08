from mock import Mock


class MockCodeEditor(object):
    def __init__(self):
        self.setFocus = Mock()
        self.findFirst = Mock()
        self.findNext = Mock()
        self.hasSelectedText = Mock()
        self.selectedText = Mock()
        self.replace = Mock()
        self.replaceAll = Mock()
