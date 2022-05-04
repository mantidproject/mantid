from numpy import array, cross, sqrt, sum, mean, zeros, diff, dot


class CutRepresentation:
    def __init__(self, canvas, notify_on_release, xmin, xmax, ymin, ymax, thickness):
        self.notify_on_release = notify_on_release
        self.canvas = canvas
        self.ax = canvas.figure.axes[0]
        self.thickness = thickness
        self.start = self.ax.plot(xmin, ymin, 'ow', label='start')[0]
        self.end = self.ax.plot(xmax, ymax, 'ow', label='end')[0]
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
        u = array([xmax - xmin, ymax - ymin, 0])
        w = cross(u, [0, 0, 1])[0:-1]
        what = w / sqrt(sum(w ** 2))
        return what

    def draw_line(self):
        xmin, xmax, ymin, ymax = self.get_start_end_points()
        self.mid = self.ax.plot(mean([xmin, xmax]), mean([ymin, ymax]),
                                label='mid', marker='o', color='w', markerfacecolor='w')[0]
        self.line = self.ax.plot([xmin, xmax], [ymin, ymax], '-w')[0]

    def draw_box(self):
        xmin, xmax, ymin, ymax = self.get_start_end_points()
        start = array([xmin, ymin])
        end = array([xmax, ymax])
        vec = self.get_perp_dir()
        points = zeros((5, 2))
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
            dx = diff(self.ax.get_xlim())[0]
            dy = diff(self.ax.get_ylim())[0]
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
                        self.thickness = 2 * abs(dot(vec, [dx, dy]))
                else:
                    self.current_artist.set_data([event.xdata], [event.ydata])
            self.draw()  # should draw artists rather than remove and re-plot

    def on_release(self, event):
        if event.inaxes == self.ax and self.current_artist is not None:
            self.current_artist = None
            if self.end.get_xdata()[0] < self.start.get_xdata()[0]:
                self.start, self.end = self.end, self.start
            self.notify_on_release(*self.get_start_end_points(), self.thickness)
