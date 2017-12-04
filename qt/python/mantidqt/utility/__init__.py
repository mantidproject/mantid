from contextlib import contextmanager


@contextmanager
def block_signals(widget):
    """
    A context manager that helps to block widget's signals temporarily. Usage:

        with block_signals(widget):
            widget.do_actions_that_emit_signals()

    :param widget: A Qt widget signals from which should be blocked.
    """
    widget.blockSignals(True)
    yield
    widget.blockSignals(False)
