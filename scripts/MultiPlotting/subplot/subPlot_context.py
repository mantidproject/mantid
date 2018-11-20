from __future__ import (absolute_import, division, print_function)
from six import iteritems
from mantid import plots
from copy import copy

# use this to manage lines and workspaces directly

# subplot context ... 
class subPlotContext(object):

    def __init__(self, name):
        self.name = name
        self._ws = {}
        self._lines = {}
        self._specNum = {}
        self._colours = {}

    # need to add if errors
    def addLine(self, subplot, ws, specNum=1):
        #make plot
        line, = plots.plotfunctions.plot(subplot,ws,specNum=specNum)
        label = line.get_label()
        # line will be a list - will include error bars
        self._specNum[label] = specNum
        if ws not in self._ws.keys():
            self._ws[ws] = [label]
        else:
            self._ws[ws].append(label)
        self.lines[label] = [line]

    def change_errors(self,subplot,state):
         for label in self.lines.keys():
             self.redraw(subplot,state,label)

    def get_ws(self,label):
        for ws in self._ws.keys():
            if label in self._ws[ws]:
                return ws

    def redraw(self,subplot,with_errors,label):
        colour = copy(self.lines[label][0].get_color())
        marker = copy(self.lines[label][0].get_marker())
        for line in self.lines[label]:
            line.remove()
        del self._lines[label]
        if with_errors:
           line, cap_lines, bar_lines = plots.plotfunctions.errorbar(subplot,self.get_ws(label),specNum=self.specNum[label],color=colour,marker=marker,label = label)
           all_lines = [line]
           all_lines.extend(cap_lines)
           all_lines.extend(bar_lines)
           self._lines[label] = all_lines
        else:
           line, = plots.plotfunctions.plot(subplot,self.get_ws(label),specNum=self.specNum[label],color=colour,marker=marker,label = label)
           self._lines[label] = [line]

    @property
    def lines(self):
        return self._lines

    @property
    def ws(self):
        return self._ws

    @property
    def specNum(self):
        return self._specNum
		
    @property
    def colours(self):
        return self._colours

    # seems to work - need to add remove specNum and ws.
    def removeLine(self, name):
        lines = self._lines[name]
        for line in lines:
            line.remove()
            del line
        del self._lines[name]
        del self._specNum[name]
        del self.colours[name]
        # remove line for ws
        to_delete = []
        for key, list in iteritems(self._ws):
            if name in list:
                list.remove(name)
                if len(list) == 0:
                    to_delete.append(key)
        for key in to_delete:
            del self._ws[key]

    def delete(self):
        keys = self._lines.keys()
        for label in keys:
            self.removeLine(label)
