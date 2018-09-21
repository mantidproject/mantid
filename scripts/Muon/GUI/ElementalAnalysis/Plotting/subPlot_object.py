from __future__ import (absolute_import, division, print_function)
from six import iteritems

# use this to manage lines and workspaces directly 
class subPlot(object):
    def __init__(self,name):
        self.name = name
        self._ws = {}
        self._lines ={}
        self._specNum = {}

    def addLine(self,label,lines,ws,specNum=1):
         # line will be a list - will include error bars         
         self._lines[label] = lines
         self._specNum[label] = specNum 
         if ws not in self._ws.keys():
            self._ws[ws] = [label]
         else:
            self._ws[ws].append(label)

    @property
    def lines(self):
        return self._lines
    @property
    def ws(self):
        return self._ws
    @property
    def specNum(self):
        return self._specNum

    #seems to work - need to add remove specNum and ws.
    def removeLine(self,name):
        lines = self._lines[name]
        for line in lines:
            line.remove()
            del line
        del self._lines[name]
        del self._specNum[name]
        # remove line for ws
        to_delete =[]
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
