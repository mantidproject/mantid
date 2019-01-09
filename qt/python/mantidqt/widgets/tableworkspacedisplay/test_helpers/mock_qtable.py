from mock import Mock


class MockQTable:
    def __init__(self):
        self.setItem = Mock()
        self.setRowCount = Mock()
