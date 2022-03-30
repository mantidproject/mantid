from qtpy.QtWidgets import QVBoxLayout, QWidget, QTableWidget, QHeaderView, QTableWidgetItem
from qtpy.QtCore import Qt
from qtpy.QtGui import QColor
from mantid.plots.plotfunctions import create_subplots
from mantidqt.MPLwidgets import FigureCanvas
from mantidqt.plotting.mantid_navigation_toolbar import MantidNavigationToolbar
import numpy as np
from mantid.simpleapi import AnalysisDataService as ADS


class CutViewerView(QWidget):
    """Displays a table view of the PeaksWorkspace along with controls
    to interact with the peaks.
    """
    def __init__(self, canvas, sliceinfo_provider, parent=None):
        """
        :param painter: An object responsible for drawing the representation of the cut
        :param sliceinfo_provider: An object responsible for providing access to current slice information
        :param parent: An optional parent widget
        """
        super().__init__(parent)
        self.layout = None
        self.table = None
        self.figure = None
        self.figure_layout = None
        self.cut_rep = None
        self.xvec = None
        self.yvec = None
        self.canvas = canvas
        self._sliceinfo_provider = sliceinfo_provider
        self._setup_ui()
        self._init_slice_table()
        self.table.cellChanged.connect(self.on_cell_changed)
        self.hide()  # hide initially as visibility is toggled

    def show(self):
        super().show()
        if len(self.figure.axes[0].tracked_workspaces) == 0:
            self.send_bin_params()  # make initial cut
        self.plot_line()

    def hide(self):
        super().hide()
        if self.cut_rep is not None:
            self.cut_rep.remove()
            self.cut_rep = None

    def reset_table_data(self):
        self.table.blockSignals(True)
        # write proj matrix to table
        proj_matrix = self._sliceinfo_provider.get_proj_matrix().T  # .T so now each basis vector is a row as in table
        dims = self._sliceinfo_provider.get_dimensions()
        states = dims.get_states()
        states[states.index(None)] = 2  # set last index as out of plane dimension
        for idim in range(proj_matrix.shape[0]):
            self._write_vector_to_table(states[idim], proj_matrix[idim, :])
        # write bin params for cut along horizontal axis (default)
        bin_params = dims.get_bin_params()  # nbins except last element which is integration width
        axlims = self._sliceinfo_provider.get_axes_limits()
        for irow in range(self.table.rowCount()-1):
            start, stop = axlims[irow]
            if irow == 0:
                padding_frac = 0.25
                nbins = 2*padding_frac*bin_params[states.index(irow)]*(stop-start)
                padding = padding_frac*(stop-start)
                start = start + padding
                stop = stop - padding
            else:
                nbins = 1
                step = 2*(stop-start) / bin_params[states.index(irow)]  # width = 2*step = 4*bin_width
                cen = (stop + start) / 2
                start, stop = cen-step, cen+step
            self.table.item(irow, 5).setData(Qt.EditRole, int(nbins))  # nbins
            self.table.item(irow, 3).setData(Qt.EditRole, float(start))  # start
            self.table.item(irow, 4).setData(Qt.EditRole, float(stop))  # stop
            self.table.item(irow, 6).setData(Qt.EditRole, float((stop - start) / nbins))  # step/width
        self.update_slicepoint()
        self.table.blockSignals(False)
        self.send_bin_params()
        self.plot_line()

    def update_slicepoint(self):
        dims = self._sliceinfo_provider.get_dimensions()
        width = dims.get_bin_params()[dims.get_states().index(None)]
        slicept = self._sliceinfo_provider.get_sliceinfo().z_value
        self.table.item(2, 3).setData(Qt.EditRole, float(slicept - width/2))  # start
        self.table.item(2, 4).setData(Qt.EditRole, float(slicept + width/2))  # stop
        self.table.item(2, 6).setData(Qt.EditRole, float(width))  # step (i.e. width)

    def plot_cut_ws(self, wsname):
        if len(self.figure.axes[0].tracked_workspaces) == 0:
            self.figure.axes[0].errorbar(ADS.retrieve(wsname), wkspIndex=None, marker='o', capsize=2, color='k',
                                         markersize=3)
        self.figure.axes[0].ignore_existing_data_limits = True
        self.figure.axes[0].autoscale_view()

        def match(artist):
            return artist.__module__ == "matplotlib.text"

        for textobj in self.figure.findobj(match=match):
            textobj.set_fontsize(8)
        self.figure.canvas.draw()
        self.figure.tight_layout()

    def read_bin_params_from_table(self):
        vectors = np.zeros((3, 3), dtype=float)
        extents = np.zeros((2, 3), dtype=float)
        nbins = np.zeros(3, dtype=int)
        for ivec in range(vectors.shape[0]):
            for icol in range(vectors.shape[0]):
                vectors[ivec, icol] = float(self.table.item(ivec, icol).text())
            extents[0, ivec] = float(self.table.item(ivec, 3).text())  # start
            extents[1, ivec] = float(self.table.item(ivec, 4).text())  # stop
            nbins[ivec] = int(self.table.item(ivec, 5).text())
        return vectors, extents, nbins

    def validate_table(self, irow, icol):
        vectors, extents, nbins = self.read_bin_params_from_table()
        self.table.blockSignals(True)
        ivec = int(not bool(irow))  # index of u1 or u2 - which ever not changed (3rd row not editable)
        if icol < 3:
            # choose a new vector in plane that is not a linear combination of two other
            vectors[ivec] = np.cross(vectors[irow], vectors[2])
            self._write_vector_to_table(ivec, vectors[ivec])
        elif icol == 3 or icol == 4:
            # extents changed - adjust width based on nbins
            self.table.item(irow, 6).setData(Qt.EditRole,
                                             float((extents[1, irow] - extents[0, irow]) / nbins[irow]))
        elif icol == 5:
            # nbins changed - adjust step
            if nbins[irow] < 1:
                nbins[irow] = 1
                self.table.item(irow, 5).setData(Qt.EditRole, int(nbins[irow]))  # nbins
            self.table.item(irow, 6).setData(Qt.EditRole, float((extents[1, irow] - extents[0, irow]) / nbins[irow]))
            if nbins[irow] == 1 and nbins[ivec] == 1:
                nbins[ivec] = 100
            elif nbins[irow] > 1 and nbins[ivec] > 1:
                nbins[ivec] = 1
            self.table.item(ivec, 5).setData(Qt.EditRole, int(nbins[ivec]))
            self.table.item(ivec, 6).setData(Qt.EditRole,
                                             float((extents[1, ivec] - extents[0, ivec]) / nbins[ivec]))
        elif icol == 6:
            # step changed - adjust nbins
            step = float(self.table.item(irow, 6).text())
            if step < 0:
                step = abs(step)
                self.table.item(irow, 6).setData(Qt.EditRole, step)
            nbins[irow] = (extents[1, irow] - extents[0, irow]) / step
            if nbins[irow] < 1:
                nbins[irow] = 1
                self.table.item(ivec, 6).setData(Qt.EditRole, float((extents[1, irow] - extents[0, irow])))
            if nbins[irow] % 1 > 0:
                extents[irow, 1] = extents[irow, 1] - (nbins[irow] % 1) * step  # so integer number of bins
                self.table.item(irow, 4).setData(Qt.EditRole, float(extents[irow, 1]))
            self.table.item(irow, 5).setData(Qt.EditRole, int(nbins[irow]))  # nbins
        self.table.blockSignals(False)

    def on_cell_changed(self, irow, icol):
        self.validate_table(irow, icol)
        self.send_bin_params()
        self.plot_line()

    def send_bin_params(self):
        vectors, extents, nbins = self.read_bin_params_from_table()
        if (extents[-1, :] - extents[0, :] > 0).all() and np.sum(nbins > 1) == 1 \
                and not np.isclose(np.linalg.det(vectors), 0.0):
            self._sliceinfo_provider.perform_non_axis_aligned_cut(vectors, extents.flatten(order='F'), nbins)
        else:
            print('BINMD args not valid!')

    def plot_line(self):
        # find vectors corresponding to x and y axes
        proj_matrix = self._sliceinfo_provider.get_proj_matrix().T  # .T so now each basis vector is a row as in table
        dims = self._sliceinfo_provider.get_dimensions()
        states = dims.get_states()
        self.xvec = proj_matrix[states.index(0), :]
        self.xvec = self.xvec/np.sqrt(np.sum(self.xvec**2))
        self.yvec = proj_matrix[states.index(1), :]
        self.yvec = self.yvec / np.sqrt(np.sum(self.yvec ** 2))
        # find x/y coord of start/end point of cut
        vectors, extents, nbins = self.read_bin_params_from_table()
        cens = np.mean(extents, axis=0)  # in u{1..3} basis
        icut = np.where(nbins > 1)[0][0]  # index of x-axis
        ivecs = list(range(len(vectors)))
        ivecs.pop(icut)
        zero_vec = np.zeros(vectors[0].shape) # position at  0 along cut axis
        for ivec in ivecs:
            zero_vec = zero_vec + cens[ivec]*vectors[ivec]
        start = zero_vec + extents[0, icut] * vectors[icut, :]
        end = zero_vec + extents[1, icut] * vectors[icut, :]
        xmin = np.dot(start, self.xvec)
        xmax = np.dot(end, self.xvec)
        ymin = np.dot(start, self.yvec)
        ymax = np.dot(end, self.yvec)
        # get thickness of cut defined for unit vector perp to cut (so scale by magnitude of vector in the table)
        iint = nbins[:-1].tolist().index(1)
        thickness = (extents[1, iint]-extents[0, iint])*np.sqrt(np.sum(vectors[iint, :]**2))
        if self.cut_rep is not None:
            self.cut_rep.remove()
        self.cut_rep = CutRepresentation(self, self.canvas, xmin, xmax, ymin, ymax, thickness)

    def get_coords_from_cut_representation(self):
        if self.cut_rep is None:
            return
        # get vectors in u{1..3} basis
        xmin, xmax, ymin, ymax = self.cut_rep.get_start_end_points()
        start = xmin*self.xvec + ymin*self.yvec
        stop = xmax*self.xvec + ymax*self.yvec
        u1 = stop - start
        u1 = u1 / np.sqrt(np.sum(u1 ** 2))
        u1_min, u1_max = np.dot(start, u1), np.dot(stop, u1)
        # get integrated dim
        vectors, _, _ = self.read_bin_params_from_table()
        u2 = np.cross(u1, vectors[-1, :])
        u2 = u2 / np.sqrt(np.sum(u2 ** 2))
        u2_cen = np.dot(start, u2)
        u2_step = self.cut_rep.thickness
        self.table.blockSignals(True)
        self._write_vector_to_table(0, u1)
        self.table.item(0, 3).setData(Qt.EditRole, float(u1_min))
        self.table.item(0, 4).setData(Qt.EditRole, float(u1_max))
        self.table.item(0, 5).setData(Qt.EditRole, int(50))
        self.table.item(1, 6).setData(Qt.EditRole, float((u1_max-u1_min)/50))
        self._write_vector_to_table(1, u2)
        self.table.item(1, 3).setData(Qt.EditRole, float(u2_cen - u2_step/2))
        self.table.item(1, 4).setData(Qt.EditRole, float(u2_cen + u2_step/2))
        self.table.item(1, 5).setData(Qt.EditRole, int(1))
        self.table.item(1, 6).setData(Qt.EditRole, float(u2_step))
        self.table.blockSignals(False)
        self.send_bin_params()

    # private api
    def _setup_ui(self):
        """
        Arrange the widgets on the window
        """
        self.layout = QVBoxLayout()
        self.layout.sizeHint()
        self.layout.setContentsMargins(5, 0, 0, 0)
        self.layout.setSpacing(0)

        self._setup_table_widget()
        self._setup_figure_widget()
        self.setLayout(self.layout)

    def _setup_table_widget(self):
        """
        Make a table showing
        :return: A QTableWidget object which will contain plot widgets
        """
        table_widget = QTableWidget(3, 7, self)
        table_widget.setVerticalHeaderLabels(['u1', 'u2', 'u3'])
        table_widget.setHorizontalHeaderLabels(['a*', 'b*', 'c*', 'start', 'stop', 'nbins', 'step'])
        table_widget.setFixedHeight(table_widget.verticalHeader().defaultSectionSize()*(table_widget.rowCount()+1))  # +1 to include headers
        for icol in range(table_widget.columnCount()):
            table_widget.setColumnWidth(icol, 50)
        table_widget.horizontalHeader().setSectionResizeMode(QHeaderView.Stretch)
        table_widget.setVerticalScrollBarPolicy(Qt.ScrollBarAlwaysOff)

        self.table = table_widget
        self.layout.addWidget(self.table)

    def _setup_figure_widget(self):
        fig, _, _, _ = create_subplots(1)
        fig.axes[0].autoscale(enable=True, tight=False)
        self.figure = fig
        self.figure.canvas = FigureCanvas(self.figure)
        toolbar = MantidNavigationToolbar(self.figure.canvas, self)
        self.figure_layout = QVBoxLayout()
        self.figure_layout.addWidget(toolbar)
        self.figure_layout.addWidget(self.figure.canvas)
        self.layout.addLayout(self.figure_layout)

    def _init_slice_table(self):
        for icol in range(self.table.columnCount()):
            for irow in range(self.table.rowCount()):
                item = QTableWidgetItem()
                if icol == 5:
                    item.setData(Qt.EditRole, int(1))
                else:
                    item.setData(Qt.EditRole, float(1))
                if irow == self.table.rowCount()-1:
                    item.setFlags(item.flags() ^ Qt.ItemIsEditable)  # disable editing in last row (out of plane dim)
                    item.setBackground(QColor(250, 250, 250))
                else:
                    item.setFlags(item.flags() | Qt.ItemIsEditable)
                self.table.setItem(irow, icol, item)
        self.reset_table_data()

    def _write_vector_to_table(self, irow, vector):
        for icol in range(len(vector)):
            self.table.item(irow, icol).setData(Qt.EditRole, float(vector[icol]))


class CutRepresentation:
    def __init__(self, view, canvas, xmin, xmax, ymin, ymax, thickness):
        self.view = view
        self.canvas = canvas
        self.ax = canvas.figure.axes[0]
        self.thickness = thickness
        self.start = self.ax.plot(xmin, ymin, 'ow', label='start')[0]  # , picker=True)
        self.end = self.ax.plot(xmax, ymax, 'ow', label='end')[0]  # , picker=True)
        self.line = None
        self.mid = None
        self.box = None
        self.mid_box_top = None
        self.mid_box_bot = None
        self.current_artist = None
        self.cid_release = self.canvas.mpl_connect('button_release_event', self.on_release)
        self.cid_press = self.canvas.mpl_connect('button_press_event', self.on_press)
        self.cid_motion = self.canvas.mpl_connect('motion_notify_event', self.on_motion)
        self.draw()

    def remove(self):
        self.clear_lines(all_lines=True)
        for cid in [self.cid_release, self.cid_press, self.cid_motion]:
            self.canvas.mpl_disconnect(cid)

    def draw(self):
        self.draw_line()
        self.draw_box()
        self.canvas.draw()

    def get_start_end_points(self):
        xmin, xmax = self.start.get_xdata()[0], self.end.get_xdata()[0]
        ymin, ymax = self.start.get_ydata()[0], self.end.get_ydata()[0]
        return xmin, xmax, ymin, ymax

    def get_mid_point(self):
        xmin, xmax, ymin, ymax = self.get_start_end_points()
        return 0.5 * (xmin + xmax), 0.5 * (ymin + ymax)

    def get_perp_dir(self):
        # vector perp to line
        xmin, xmax, ymin, ymax = self.get_start_end_points()
        u = np.array([xmax - xmin, ymax - ymin, 0])
        w = np.cross(u, [0, 0, 1])[0:-1]
        what = w / np.sqrt(np.sum(w ** 2))
        return what

    def draw_line(self):
        xmin, xmax, ymin, ymax = self.get_start_end_points()
        self.mid = self.ax.plot(np.mean([xmin, xmax]), np.mean([ymin, ymax]),
                                label='mid', marker='o', color='w', markerfacecolor='w')[0]
        self.line = self.ax.plot([xmin, xmax], [ymin, ymax], '-w')[0]

    def draw_box(self):
        xmin, xmax, ymin, ymax = self.get_start_end_points()
        start = np.array([xmin, ymin])
        end = np.array([xmax, ymax])
        vec = self.get_perp_dir()
        points = np.zeros((5, 2))
        points[0, :] = start + self.thickness * vec / 2
        points[1, :] = start - self.thickness * vec / 2
        points[2, :] = end - self.thickness * vec / 2
        points[3, :] = end + self.thickness * vec / 2
        points[4, :] = points[0, :]  # close the loop
        self.box = self.ax.plot(points[:, 0], points[:, 1], '--r')[0]
        # plot mid points
        mid = 0.5 * (start + end)
        mid_top = mid + self.thickness * vec / 2
        mid_bot = mid - self.thickness * vec / 2
        self.mid_box_top = self.ax.plot(mid_top[0], mid_top[1], 'or', label='mid_box_top',
                                        markerfacecolor='w')[0]
        self.mid_box_bot = self.ax.plot(mid_bot[0], mid_bot[1], 'or', label='mid_box_bot',
                                        markerfacecolor='w')[0]

    def clear_lines(self, all_lines=False):
        lines_to_clear = [self.mid, self.line, self.box, self.mid_box_top, self.mid_box_bot]
        if all_lines:
            lines_to_clear.extend([self.start, self.end])  # normally don't delete these as artist data kept updated
        for line in lines_to_clear:
            if line in self.ax.lines:
                self.ax.lines.remove(line)

    def on_press(self, event):
        if event.inaxes == self.ax and self.current_artist is None:
            x, y = event.xdata, event.ydata
            dx = np.diff(self.ax.get_xlim())[0]
            dy = np.diff(self.ax.get_ylim())[0]
            for line in [self.start, self.end, self.mid, self.mid_box_top, self.mid_box_bot]:
                if abs(x - line.get_xdata()[0]) < dx / 100 and abs(y - line.get_ydata()[0]) < dy / 100:
                    self.current_artist = line
                    break

    def on_motion(self, event):
        if event.inaxes == self.ax and self.current_artist is not None:
            self.clear_lines()
            if len(self.current_artist.get_xdata()) == 1:
                if 'mid' in self.current_artist.get_label():
                    x0, y0 = self.get_mid_point()
                    dx = event.xdata - x0
                    dy = event.ydata - y0
                    if self.current_artist.get_label() == 'mid':
                        for line in [self.start, self.end]:
                            line.set_data([line.get_xdata()[0] + dx], [line.get_ydata()[0] + dy])
                    else:
                        vec = self.get_perp_dir()
                        self.thickness = 2 * abs(np.dot(vec, [dx, dy]))
                else:
                    self.current_artist.set_data([event.xdata], [event.ydata])
            self.draw()  # should draw artists rather than remove and re-plot

    def on_release(self, event):
        if event.inaxes == self.ax and self.current_artist is not None:
            self.current_artist = None
            if self.end.get_xdata()[0] < self.start.get_xdata()[0]:
                self.start, self.end = self.end, self.start
            self.view.get_coords_from_cut_representation()
