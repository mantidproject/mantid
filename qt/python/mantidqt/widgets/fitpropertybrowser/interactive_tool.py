from qtpy.QtCore import QObject, Signal, Slot
from qtpy.QtGui import QCursor
from qtpy.QtWidgets import QApplication, QMenu, QInputDialog

from markers import VerticalMarker, PeakMarker
from mouse_state_machine import StateMachine


class FitInteractiveTool(QObject):

    fit_start_x_moved = Signal(float)
    fit_end_x_moved = Signal(float)
    peak_added = Signal(int, float, float, float)
    peak_moved = Signal(int, float, float)
    peak_fwhm_changed = Signal(int, float)
    peak_type_changed = Signal(str)
    add_background_requested = Signal(str)
    add_other_requested = Signal(str)

    default_background = 'LinearBackground'

    def __init__(self, canvas, toolbar_state_checker, current_peak_type):
        super(FitInteractiveTool, self).__init__()
        self.canvas = canvas
        self.toolbar_state_checker = toolbar_state_checker
        ax = canvas.figure.get_axes()[0]
        self.ax = ax
        xlim = ax.get_xlim()
        dx = (xlim[1] - xlim[0]) / 20.
        start_x = xlim[0] + dx
        end_x = xlim[1] - dx
        self.fit_start_x = VerticalMarker(canvas, 'green', start_x)
        self.fit_end_x = VerticalMarker(canvas, 'green', end_x)

        self.fit_start_x.x_moved.connect(self.fit_start_x_moved)
        self.fit_end_x.x_moved.connect(self.fit_end_x_moved)

        self.peak_markers = []
        self.selected_peak = None
        self.peak_names = []
        self.current_peak_type = current_peak_type
        self.fwhm = dx
        self.background_names = []
        self.other_names = []

        self._cids = []
        self._cids.append(canvas.mpl_connect('draw_event', self.draw_callback))
        self._cids.append(canvas.mpl_connect('motion_notify_event', self.motion_notify_callback))
        self._cids.append(canvas.mpl_connect('button_press_event', self.button_press_callback))
        self._cids.append(canvas.mpl_connect('button_release_event', self.button_release_callback))

        self.mouse_state = StateMachine(self)
        self._override_cursor = False

    def disconnect(self):
        QObject.disconnect(self)
        for cid in self._cids:
            self.canvas.mpl_disconnect(cid)
        self.fit_start_x.remove()
        self.fit_end_x.remove()

    def draw_callback(self, event):
        if self.fit_start_x.x > self.fit_end_x.x:
            x = self.fit_start_x.x
            self.fit_start_x.x = self.fit_end_x.x
            self.fit_end_x.x = x
        self.fit_start_x.redraw()
        self.fit_end_x.redraw()
        for pm in self.peak_markers:
            pm.redraw()

    def get_override_cursor(self, x, y):
        cursor = self.fit_start_x.override_cursor(x, y)
        if cursor is None:
            cursor = self.fit_end_x.override_cursor(x, y)
        if cursor is None:
            for pm in self.peak_markers:
                cursor = pm.override_cursor(x, y)
                if cursor is not None:
                    break
        return cursor

    @property
    def override_cursor(self):
        return self._override_cursor

    @override_cursor.setter
    def override_cursor(self, cursor):
        self._override_cursor = cursor
        QApplication.restoreOverrideCursor()
        if cursor is not None:
            QApplication.setOverrideCursor(cursor)

    def motion_notify_callback(self, event):
        self.mouse_state.motion_notify_callback(event)

    def button_press_callback(self, event):
        self.mouse_state.button_press_callback(event)

    def button_release_callback(self, event):
        self.mouse_state.button_release_callback(event)

    def move_markers(self, event):
        x, y = event.xdata, event.ydata
        if x is None or y is None:
            return
        self.override_cursor = self.get_override_cursor(x, y)
        should_redraw = self.fit_start_x.mouse_move(x)
        should_redraw = self.fit_end_x.mouse_move(x) or should_redraw
        for pm in self.peak_markers:
            should_redraw = pm.mouse_move(x, y) or should_redraw
        if should_redraw:
            self.canvas.draw()

    def start_move_markers(self, event):
        x = event.xdata
        y = event.ydata
        if x is None or y is None:
            return
        self.fit_start_x.mouse_move_start(x, y)
        self.fit_end_x.mouse_move_start(x, y)
        selected_peak = None
        for pm in self.peak_markers:
            pm.mouse_move_start(x, y)
            if pm.is_moving:
                selected_peak = pm
        if selected_peak is not None:
            self.select_peak(selected_peak)
            self.canvas.draw()

    def stop_move_markers(self, event):
        self.fit_start_x.mouse_move_stop()
        self.fit_end_x.mouse_move_stop()
        for pm in self.peak_markers:
            pm.mouse_move_stop()

    def move_start_x(self, x):
        if x is not None:
            self.fit_start_x.x = x
            self.canvas.draw()

    def move_end_x(self, x):
        if x is not None:
            self.fit_end_x.x = x
            self.canvas.draw()

    def _make_peak_id(self):
        ids = set([pm.peak_id for pm in self.peak_markers])
        n = 0
        for i in range(len(ids)):
            if i in ids:
                if i > n:
                    n = i
            else:
                return i
        return n + 1

    def add_default_peak(self):
        self.mouse_state.transition_to('add_peak')

    def add_peak_dialog(self):
        selected_name = QInputDialog.getItem(self.canvas, 'Fit', 'Select peak function', self.peak_names,
                                             self.peak_names.index(self.current_peak_type), False)
        if selected_name[1]:
            self.peak_type_changed.emit(selected_name[0])
            self.mouse_state.transition_to('add_peak')

    def add_background_dialog(self):
        current_index = self.background_names.index(self.default_background)
        if current_index < 0:
            current_index = 0
        selected_name = QInputDialog.getItem(self.canvas, 'Fit', 'Select background function', self.background_names,
                                             current_index, False)
        if selected_name[1]:
            self.add_background_requested.emit(selected_name[0])

    def add_other_dialog(self):
        selected_name = QInputDialog.getItem(self.canvas, 'Fit', 'Select function', self.other_names, 0, False)
        if selected_name[1]:
            self.add_other_requested.emit(selected_name[0])

    def add_peak(self, x, y_top, y_bottom=0.0):
        peak_id = self._make_peak_id()
        peak = PeakMarker(self.canvas, peak_id, x, y_top, y_bottom, fwhm=self.fwhm)
        peak.peak_moved.connect(self.peak_moved)
        peak.fwhm_changed.connect(self.peak_fwhm_changed_slot)
        self.peak_markers.append(peak)
        self.select_peak(peak)
        self.canvas.draw()
        self.peak_added.emit(peak_id, x, peak.height(), peak.fwhm())

    def update_peak(self, peak_id, centre, height, fwhm):
        for pm in self.peak_markers:
            if pm.peak_id == peak_id:
                pm.update_peak(centre, height, fwhm)
        self.canvas.draw()

    def select_peak(self, peak):
        self.selected_peak = None
        for pm in self.peak_markers:
            if peak == pm:
                pm.select()
                self.selected_peak = peak
            else:
                pm.deselect()

    @Slot(int, float)
    def peak_fwhm_changed_slot(self, peak_id, fwhm):
        self.fwhm = fwhm
        self.peak_fwhm_changed.emit(peak_id, fwhm)

    def get_transform(self):
        return self.fit_start_x.patch.get_transform()

    def show_context_menu(self, peak_names, current_peak_type, background_names, other_names):
        self.peak_names = peak_names
        self.current_peak_type = current_peak_type
        self.background_names = background_names
        self.other_names = other_names
        if not self.toolbar_state_checker.is_tool_active():
            menu = QMenu()
            menu.addAction("Add peak", self.add_default_peak)
            menu.addAction("Select peak type", self.add_peak_dialog)
            menu.addAction("Add background", self.add_background_dialog)
            menu.addAction("Add other function", self.add_other_dialog)
            menu.exec_(QCursor.pos())
