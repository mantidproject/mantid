# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
from numpy import array, cross, sqrt, sum, mean, zeros, diff


class CutRepresentation:
    def __init__(self, canvas, notify_on_release, xmin_c, xmax_c, ymin_c, ymax_c, thickness_c, transform):
        self.notify_on_release = notify_on_release
        self.canvas = canvas
        self.ax = canvas.figure.axes[0]
        self.transform = transform

        # store quantities in crystal basis
        self.xmin_c = xmin_c
        self.xmax_c = xmax_c
        self.ymin_c = ymin_c
        self.ymax_c = ymax_c
        self.thickness_c = thickness_c

        self.line = None
        self.mid = None
        self.box = None
        self.mid_box_top = None
        self.mid_box_bot = None
        self.current_artist = None
        self.cid_release = self.canvas.mpl_connect("button_release_event", self.on_release)
        self.cid_press = self.canvas.mpl_connect("button_press_event", self.on_press)
        self.cid_motion = self.canvas.mpl_connect("motion_notify_event", self.on_motion)
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
        xmin_d, xmax_d = self.start.get_xdata()[0], self.end.get_xdata()[0]
        ymin_d, ymax_d = self.start.get_ydata()[0], self.end.get_ydata()[0]
        return xmin_d, xmax_d, ymin_d, ymax_d

    def get_mid_point(self):
        xmin_d, xmax_d, ymin_d, ymax_d = self.get_start_end_points()
        return 0.5 * (xmin_d + xmax_d), 0.5 * (ymin_d + ymax_d)

    def get_perp_dir_crystal(self):
        # vector perp to line in crystal basis
        u = array([self.xmax_c - self.xmin_c, self.ymax_c - self.ymin_c, 0])
        w = cross(u, [0, 0, 1])[0:-1]
        return w / sqrt(sum(w**2))

    def get_perp_dir(self):
        w = self.get_perp_dir_crystal()

        # convert perp vector from crystal basis for display - assumes transformation leave origin unaltered
        to_display = self.transform.tr
        perp_display_x, perp_display_y = to_display(w[0], w[1])
        w_display = array([perp_display_x, perp_display_y])

        return w_display / sqrt(sum(w_display**2))

    def draw_line(self):
        to_display = self.transform.tr
        xmin_d, ymin_d = to_display(self.xmin_c, self.ymin_c)
        xmax_d, ymax_d = to_display(self.xmax_c, self.ymax_c)

        self.start = self.ax.plot(xmin_d, ymin_d, "ow", label="start")[0]
        self.end = self.ax.plot(xmax_d, ymax_d, "ow", label="end")[0]

        self.mid = self.ax.plot(mean([xmin_d, xmax_d]), mean([ymin_d, ymax_d]), label="mid", marker="o", color="w", markerfacecolor="w")[0]
        self.line = self.ax.plot([xmin_d, xmax_d], [ymin_d, ymax_d], "-w")[0]

    def draw_box(self):
        xmin_d, xmax_d, ymin_d, ymax_d = self.get_start_end_points()
        start = array([xmin_d, ymin_d])
        end = array([xmax_d, ymax_d])
        vec = self.get_perp_dir()

        to_display = self.transform.tr
        perp_vec = self.get_perp_dir_crystal() * self.thickness_c
        thick_x, thick_y = to_display(perp_vec[0], perp_vec[1])
        thickness_d = sqrt(pow(thick_x, 2) + pow(thick_y, 2))

        points = zeros((5, 2))
        points[0, :] = start + thickness_d * vec / 2
        points[1, :] = start - thickness_d * vec / 2
        points[2, :] = end - thickness_d * vec / 2
        points[3, :] = end + thickness_d * vec / 2
        points[4, :] = points[0, :]  # close the loop
        self.box = self.ax.plot(points[:, 0], points[:, 1], "--r")[0]
        # plot mid points
        mid = 0.5 * (start + end)
        mid_top = mid + thickness_d * vec / 2
        mid_bot = mid - thickness_d * vec / 2
        self.mid_box_top = self.ax.plot(mid_top[0], mid_top[1], "or", label="mid_box_top", markerfacecolor="w")[0]
        self.mid_box_bot = self.ax.plot(mid_bot[0], mid_bot[1], "or", label="mid_box_bot", markerfacecolor="w")[0]

    def clear_lines(self, all_lines=False):
        lines_to_clear = [self.mid, self.line, self.box, self.mid_box_top, self.mid_box_bot]
        if all_lines:
            lines_to_clear.extend([self.start, self.end])  # normally don't delete these as artist data kept updated
        for line in lines_to_clear:
            if line in self.ax.lines:
                line.remove()

    def on_press(self, event):
        if self.is_valid_event(event):
            x, y = event.xdata, event.ydata
            dx = diff(self.ax.get_xlim())[0]
            dy = diff(self.ax.get_ylim())[0]
            for line in [self.start, self.end, self.mid, self.mid_box_top, self.mid_box_bot]:
                if abs(x - line.get_xdata()[0]) < dx / 100 and abs(y - line.get_ydata()[0]) < dy / 100:
                    self.current_artist = line
                    break

    def on_motion(self, event):
        if self.is_valid_event(event) and self.has_current_artist():
            self.clear_lines(all_lines=True)
            from_display = self.transform.inv_tr
            if "mid" in self.current_artist.get_label():
                x0, y0 = self.get_mid_point()
                dx = event.xdata - x0
                dy = event.ydata - y0
                if self.current_artist.get_label() == "mid":
                    dx_c, dy_c = from_display(dx, dy)
                    self.xmin_c, self.xmax_c = (x + dx_c for x in (self.xmin_c, self.xmax_c))
                    self.ymin_c, self.ymax_c = (y + dy_c for y in (self.ymin_c, self.ymax_c))
                else:
                    vec = self.get_perp_dir() * 2 * sqrt(pow(dx, 2) + pow(dy, 2))
                    dx_c, dy_c = from_display(vec[0], vec[1])
                    self.thickness_c = sqrt(pow(dx_c, 2) + pow(dy_c, 2))
            elif self.current_artist.get_label() == "start":
                self.xmin_c, self.ymin_c = from_display(event.xdata, event.ydata)
            elif self.current_artist.get_label() == "end":
                self.xmax_c, self.ymax_c = from_display(event.xdata, event.ydata)
            self.draw()

    def on_release(self, event):
        if self.is_valid_event(event) and self.has_current_artist():
            self.current_artist = None
            if self.xmax_c < self.xmin_c:
                self.xmin_c, self.xmax_c = self.xmax_c, self.xmin_c
                self.ymin_c, self.ymax_c = self.ymax_c, self.ymin_c
            self.notify_on_release(self.xmin_c, self.xmax_c, self.ymin_c, self.ymax_c, self.thickness_c)

    def is_valid_event(self, event):
        return event.inaxes == self.ax

    def has_current_artist(self):
        return self.current_artist is not None
