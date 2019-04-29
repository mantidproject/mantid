# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy.QtCore import Qt
from qtpy.QtGui import QCursor
from qtpy.QtWidgets import QApplication


class StateMachine(object):
    """
    A state machine implementation to control mouse interaction with the peak editing tool.
    """

    def __init__(self, tool):
        self.tool = tool
        self.state = MoveMarkersState(self)

    def button_press_callback(self, event):
        """
        Respond to mouse press event.
        """
        if self.tool.toolbar_manager.is_tool_active():
            return
        self.state.button_press_callback(event)

    def motion_notify_callback(self, event):
        """
        Respond to mouse move event.
        """
        if self.tool.toolbar_manager.is_tool_active():
            return
        self.state.motion_notify_callback(event)

    def button_release_callback(self, event):
        """
        Respond to mouse release event.
        """
        if self.tool.toolbar_manager.is_tool_active():
            return
        self.state.button_release_callback(event)
        self.state = self.state.transition()

    def transition_to(self, state_name):
        """
        Change the state.
        :param state_name: A name of a new state.
        """
        if state_name == 'add_peak':
            self.state = AddPeakState(self)
        elif state_name == 'move_markers':
            self.state = MoveMarkersState(self)
        else:
            raise RuntimeError('Unknown state {}'.format(state_name))


class MoveMarkersState(object):
    """
    A state that controls marker movement.
    """

    def __init__(self, machine):
        self.machine = machine
        self.tool = machine.tool

    def button_press_callback(self, event):
        """Override base class method"""
        if event.button == 1:
            self.tool.start_move_markers(event)

    def motion_notify_callback(self, event):
        """Override base class method"""
        self.tool.move_markers(event)

    def button_release_callback(self, event):
        """Override base class method"""
        if event.button == 1:
            self.tool.stop_move_markers(event)

    def transition(self):
        """
        Get the state the machine should return to after the mouse button release: self
        """
        return self


class AddPeakState(object):
    """
    A state that controls adding a new peak.
    """

    def __init__(self, machine):
        self.machine = machine
        self.tool = machine.tool
        self.cursor = None

    def button_press_callback(self, event):
        """Override base class method"""
        pass

    def motion_notify_callback(self, event):
        """Override base class method"""
        if self.cursor is None:
            self.cursor = QCursor(Qt.CrossCursor)
            QApplication.setOverrideCursor(self.cursor)

    def button_release_callback(self, event):
        """Override base class method"""
        self.tool.add_peak(event.xdata, event.ydata)

    def transition(self):
        """
        Get the state the machine should return to after the mouse button release: MoveMarkersState
        """
        QApplication.restoreOverrideCursor()
        return MoveMarkersState(self.machine)
