from __future__ import (absolute_import, division, print_function)
from six import iteritems
from mantid import plots
from copy import copy

# use this to manage lines and workspaces directly
# may need to store if autoscale and errors in here
# to recover when using the plot selector
# subplot context ... 
class subPlotContext(object):

    def __init__(self, name,subplot):
        self.name = name
        self._subplot = subplot
        self._ws = {}
        self._lines = {}
        self._specNum = {}
        self._errors = False

    # need to add if errors
    def addLine(self, ws, specNum=1):
        #make plot/get label
        line, = plots.plotfunctions.plot(self._subplot,ws,specNum=specNum)
        label = line.get_label()
        if self._errors: 
           line, cap_lines, bar_lines = plots.plotfunctions.errorbar(self._subplot,ws,specNum=specNum,label = label)
           all_lines = [line]
           all_lines.extend(cap_lines)
           all_lines.extend(bar_lines)
           self._lines[label] = all_lines
        else:
		   self._lines[label] = [line]
        # line will be a list - will include error bars
        self._specNum[label] = specNum
        if ws not in self._ws.keys():
            self._ws[ws] = [label]
        else:
            self._ws[ws].append(label)


    def change_errors(self,state):
         self._errors = state
         for label in self._lines.keys():
             self.redraw(label)

    def change_auto(self,state):
         for label in self._lines.keys():
             self._subplot.autoscale(enable=state,axis="y")
             

    def get_ws(self,label):
        for ws in self._ws.keys():
            if label in self._ws[ws]:
                return ws

    def redraw(self,label):
        colour = copy(self._lines[label][0].get_color())
        marker = copy(self._lines[label][0].get_marker())
        for line in self._lines[label]:
            line.remove()
        del self._lines[label]
        if self._errors:
           line, cap_lines, bar_lines = plots.plotfunctions.errorbar(self._subplot,self.get_ws(label),specNum=self.specNum[label],color=colour,marker=marker,label = label)
           all_lines = [line]
           all_lines.extend(cap_lines)
           all_lines.extend(bar_lines)
           self._lines[label] = all_lines
        else:
           line, = plots.plotfunctions.plot(self._subplot,self.get_ws(label),specNum=self.specNum[label],color=colour,marker=marker,label = label)
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

    @property
    def errors(self):
        return self._errors

    @property
    def xbounds(self):
        return self._subplot.get_xlim()

    @property
    def ybounds(self):
        return self._subplot.get_ylim()

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
