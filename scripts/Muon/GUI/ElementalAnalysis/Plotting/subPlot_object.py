from __future__ import (absolute_import, division, print_function)

# use this to manage lines and workspaces directly 
class subPlot(object):
    def __init__(self,name):
        self.name = name
        self._ws = {}
        self._lines ={}
        self._specNum = {}

    def addLine(self,label,lines,ws,specNum=1):
         
         self._lines[label] = lines
         self._specNum[label] = specNum 
         if ws not in self._ws.keys():
            self._ws[ws] = [label]
         else:
            self._ws[ws].append(label)

    def myprint(self):
        print(self.name)
        print(self._lines)
        for ws in self._ws.keys():
            print(self._ws[ws])

    @property
    def lines(self):
        return self._lines

    #seems to work
    def removeLine(self,name):
        line = self.lines[name]
        line.remove()
        del line
