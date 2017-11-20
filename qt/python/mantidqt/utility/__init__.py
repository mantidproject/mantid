class SignalBlocker(object):

    def __init__(self, widget):
        self.widget = widget

    def __enter__(self):
        self.widget.blockSignals(True)

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.widget.blockSignals(False)


def block_signals(widget):
    return SignalBlocker(widget)
