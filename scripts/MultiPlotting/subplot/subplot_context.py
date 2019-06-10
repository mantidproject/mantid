from __future__ import (absolute_import, division, print_function)
from six import iteritems
from mantid import plots
from copy import copy

# use this to manage lines and workspaces directly
# store errors in here
# to recover plot when using the plot selector


class subplotContext(object):

    def __init__(self, name, subplot):
        self.name = name
        self._subplot = subplot
        self._ws = {}
        self._lines = {}
        self._specNum = {}
        self._errors = False
        self._vLines = {}
        self._labelObjects = {}
        self._labels = {}

    def update_gridspec(self, gridspec, figure, j):
        tmp = gridspec[j].get_position(figure)
        self._subplot.set_position(tmp)
        self._subplot.set_subplotspec(gridspec[j])

    def add_vline(self, xvalue, name):
        self._vLines[name] = self._subplot.axvline(xvalue, 0, 1)

    def add_annotate(self, label):
        self._labelObjects[label.text] = label
        x_range = self._subplot.get_xlim()
        y_range = self._subplot.get_ylim()
        if label.in_x_range(x_range):
            self._labels[label.text] = self._subplot.annotate(
                label.text,
                xy=(label.get_xval(x_range),
                    label.get_yval(y_range)),
                xycoords="axes fraction",
                rotation=label.rotation)

    def redraw_annotations(self):
        for key in list(self._labels.keys()):
            self._labels[key].remove()
            del self._labels[key]
        for label in list(self._labelObjects.keys()):
            self.add_annotate(self._labelObjects[label])

    def addLine(self, ws, specNum=1):
        # make plot/get label
        line, = plots.plotfunctions.plot(self._subplot, ws, specNum=specNum)
        label = line.get_label()
        if self._errors:
            line, cap_lines, bar_lines = plots.plotfunctions.errorbar(
                self._subplot, ws, specNum=specNum, label=label)
            all_lines = [line]
            all_lines.extend(cap_lines)
            all_lines.extend(bar_lines)
            self._lines[label] = all_lines
        else:
            # line will be a list - will include error bars
            self._lines[label] = [line]
        self._specNum[label] = specNum
        # store the ws as the key and have a list of labels that use that ws
        if ws not in self._ws.keys():
            self._ws[ws] = [label]
        else:
            self._ws[ws].append(label)

    def redraw(self, label):
        # get current colour and marker
        colour = copy(self._lines[label][0].get_color())
        marker = copy(self._lines[label][0].get_marker())
        # delete old lines
        for line in self._lines[label]:
            line.remove()
        del self._lines[label]
        # replot the line
        if self._errors:
            line, cap_lines, bar_lines = plots.plotfunctions.errorbar(self._subplot, self.get_ws(
                label), specNum=self.specNum[label], color=colour, marker=marker, label=label)
            all_lines = [line]
            all_lines.extend(cap_lines)
            all_lines.extend(bar_lines)
            self._lines[label] = all_lines
        else:
            line, = plots.plotfunctions.plot(self._subplot, self.get_ws(
                label), specNum=self.specNum[label], color=colour, marker=marker, label=label)
            self._lines[label] = [line]

    def replace_ws(self, ws):
        redraw_canvas = False
        for key in self._ws.keys():
            if key.name() == ws.name():
                redraw_canvas = True
                self._ws[ws] = self._ws.pop(key)
                for label in self._ws[ws]:
                    self.redraw(label)
        return redraw_canvas

    def change_errors(self, state):
        self._errors = state
        for label in list(self._lines.keys()):
            self.redraw(label)

    def change_auto(self, state):
        for label in list(self._lines.keys()):
            self._subplot.autoscale(enable=state, axis="y")
            self.redraw_annotations()

    @property
    def size(self):
        return len(self._lines)

    @property
    def lines(self):
        return self._lines

    @property
    def vlines(self):
        # only return unprotected vlines and annotations
        vlines = []
        for name in list(self._labelObjects.keys()):
            if not self._labelObjects[name].protected:
                vlines.append(name)
        for name in list(self._vLines.keys()):
            if name not in self._labelObjects.keys():
                vlines.append(name)
        return vlines

    @property
    def ws(self):
        return self._ws

    @property
    def specNum(self):
        return self._specNum

    @property
    def errors(self):
        return self._errors

    @property
    def xbounds(self):
        return self._subplot.get_xlim()

    @property
    def ybounds(self):
        return self._subplot.get_ylim()

    def get_ws(self, label):
        for ws in self._ws.keys():
            if label in self._ws[ws]:
                return ws

    def removeLine(self, name):
        if name in list(self._lines.keys()):
            self.removePlotLine(name)
        if name in list(self._vLines.keys()):
            self.removeVLine(name)
        if name in list(self._labelObjects.keys()):
            self.removeLabel(name)

    def removePlotLine(self, name):
        if name not in self._lines:
            return
        lines = self._lines[name]
        for line in lines:
            line.remove()
            del line
        del self._lines[name]
        del self._specNum[name]
        # remove line for ws
        to_delete = []
        for key, list in iteritems(self._ws):
            if name in list:
                list.remove(name)
                if len(list) == 0:
                    to_delete.append(key)
        for key in to_delete:
            del self._ws[key]

    def removeVLine(self, name):
        if name not in self._vLines:
            return
        line = self._vLines[name]
        line.remove()
        del self._vLines[name]

    def removeLabel(self, name):
        self._remove_label_object(name)
        if name not in self._labels:
            return
        label = self._labels[name]
        label.remove()
        del self._labels[name]

    def _remove_label_object(self, name):
        if name not in self._labelObjects:
            return
        del self._labelObjects[name]

    def delete(self):
        keys = list(self._lines.keys())
        for label in keys:
            self.removeLine(label)
        keys = list(self._vLines.keys())
        for label in keys:
            self.removeLine(label)
        keys = list(self._labelObjects.keys())
        for label in keys:
            self.removeLabel(label)

    def get_lines_from_WS_name(self, name):
        for ws in self._ws:
            if name == ws.name():
                return self._ws[ws]
        return []
