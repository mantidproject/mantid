from qtpy.QtCore import Qt
from qtpy.QtGui import QCursor
from qtpy.QtWidgets import QApplication, QMenu


class StateMachine(object):

    def __init__(self, tool):
        self.tool = tool
        self.state = MoveMarkersState(self)

    def button_press_callback(self, event):
        if self.tool.toolbar_state_checker.is_tool_active():
            return
        self.state.button_press_callback(event)

    def motion_notify_callback(self, event):
        if self.tool.toolbar_state_checker.is_tool_active():
            return
        self.state.motion_notify_callback(event)

    def button_release_callback(self, event):
        if self.tool.toolbar_state_checker.is_tool_active():
            return
        self.state.button_release_callback(event)
        self.state = self.state.transition()

    def transition_to(self, state_name):
        if state_name == 'add_peak':
            self.state = AddPeakState(self)
        elif state_name == 'move_markers':
            self.state = MoveMarkersState(self)
        else:
            raise RuntimeError('Unknown state {}'.format(state_name))


class MoveMarkersState(object):

    def __init__(self, machine):
        self.machine = machine
        self.tool = machine.tool
        self._next = self

    def button_press_callback(self, event):
        self.tool.start_move_markers(event)

    def motion_notify_callback(self, event):
        self.tool.move_markers(event)

    def button_release_callback(self, event):
        if event.button == 1:
            self.tool.stop_move_markers(event)
        elif event.button == 3:
            menu = QMenu()
            menu.addAction("Add peak", lambda: self._set_next(AddPeakState(self.machine)))
            menu.exec_(QCursor.pos())

    def transition(self):
        return self._next

    def _set_next(self, state):
        self._next = state


class AddPeakState(object):

    def __init__(self, machine):
        self.machine = machine
        self.tool = machine.tool
        self.cursor = None

    def button_press_callback(self, event):
        pass

    def motion_notify_callback(self, event):
        if self.cursor is None:
            self.cursor = QCursor(Qt.CrossCursor)
            QApplication.setOverrideCursor(self.cursor)

    def button_release_callback(self, event):
        self.tool.add_peak(event.xdata, event.ydata)

    def transition(self):
        QApplication.restoreOverrideCursor()
        return MoveMarkersState(self.machine)
