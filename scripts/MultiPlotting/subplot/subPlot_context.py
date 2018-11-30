from __future__ import (absolute_import, division, print_function)
from six import iteritems
from mantid import plots
from copy import copy

# use this to manage lines and workspaces directly
# may need to store if autoscale and errors in here
# to recover when using the plot selector
# subplot context ... 
class subPlotContext(object):

    def __init__(self, name):
        self.name = name
        self._ws = {}
        self._lines = {}
        self._specNum = {}
        self.xbounds = []
        self.ybounds = []
        self._errors = False
        #self._auto_y_min = 1.e6
        #self._auto_y_max = -1.e6

    # need to add if errors
    def addLine(self, subplot, ws, specNum=1):
        #make plot/get label
        line, = plots.plotfunctions.plot(subplot,ws,specNum=specNum)
        label = line.get_label()
        if self._errors: 
           line, cap_lines, bar_lines = plots.plotfunctions.errorbar(subplot,ws,specNum=specNum,label = label)
           all_lines = [line]
           all_lines.extend(cap_lines)
           all_lines.extend(bar_lines)
           self._lines[label] = all_lines
        else:
		   self.lines[label] = [line]
        # line will be a list - will include error bars
        self._specNum[label] = specNum
        if ws not in self._ws.keys():
            self._ws[ws] = [label]
        else:
            self._ws[ws].append(label)

        if self.xbounds == []:
            self.xbounds = subplot.get_xlim()
        if self.ybounds == []:
            self.ybounds = subplot.get_ylim()
            #self._auto_y = subplot.get_ylim()
        #if self._auto_y_min > subplot.get_ylim()[0]:
        #    self._auto_y_min = subplot.get_ylim()[0]
        #if self._auto_y_max < subplot.get_ylim()[1]:
        #    self._auto_y_max = subplot.get_ylim()[1]

    @property
    def auto_y(self):
        return None
        #print("boo", self._auto_y_min, self._auto_y_max)

        #return [self._auto_y_min,self._auto_y_max]

    def change_errors(self,subplot,state):
         self._errors = state
         for label in self.lines.keys():
             self.redraw(subplot,label)

    def get_ws(self,label):
        for ws in self._ws.keys():
            if label in self._ws[ws]:
                return ws

    def redraw(self,subplot,label):
        colour = copy(self.lines[label][0].get_color())
        marker = copy(self.lines[label][0].get_marker())
        for line in self.lines[label]:
            line.remove()
        del self._lines[label]
        if self._errors:
           line, cap_lines, bar_lines = plots.plotfunctions.errorbar(subplot,self.get_ws(label),specNum=self.specNum[label],color=colour,marker=marker,label = label)
           all_lines = [line]
           all_lines.extend(cap_lines)
           all_lines.extend(bar_lines)
           self._lines[label] = all_lines
        else:
           line, = plots.plotfunctions.plot(subplot,self.get_ws(label),specNum=self.specNum[label],color=colour,marker=marker,label = label)
           self._lines[label] = [line]
        #update auto scale values
        #print("moo", self._auto_y_min,self._auto_y_max,subplot.get_ylim())
        # need to do something clever to record the best values
        #self._auto_y_min = subplot.get_ylim()[0]
        #self._auto_y_max = subplot.get_ylim()[1]

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
