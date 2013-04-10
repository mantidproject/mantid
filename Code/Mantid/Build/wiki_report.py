from xml.dom.minidom import getDOMImplementation

class WikiReporter:

    _time_taken = 0.0
    _failures = []
    _skipped = []
    
    def __init__(self, showSkipped=True):
        self._doc = getDOMImplementation().createDocument(None,'testsuite',None)
        self._show_skipped = showSkipped

    def reportStatus(self):
        return len(self._failures) == 0

    def getResults(self):
        # print the command line summary version of the results
        self._failures.sort()
        self._skipped.sort()
        print
        if self._show_skipped and len(self._skipped) > 0:
            print "SKIPPED:"
            for test in self._skipped:
                print test
        if len(self._failures) > 0:
            print "FAILED:"
            for test in self._failures:
                print test

        # return the xml document version
        docEl = self._doc.documentElement
        docEl.setAttribute('name','SystemTests')
        docEl.setAttribute('tests',str(len(docEl.childNodes)))
        docEl.setAttribute('failures',str(len(self._failures)))
        docEl.setAttribute('skipped', str(len(self._skipped)))
        docEl.setAttribute('time',str(self._time_taken))
        return self._doc.toxml()
    
    def __addGenericFailure__(self, contents, algorithm):
        elem = self._doc.createElement('testcase')
        elem.setAttribute('classname', 'WikiMaker')
        elem.setAttribute('name', algorithm)
        self._failures.append(algorithm)
        failEl = self._doc.createElement('failure')
        failEl.appendChild(self._doc.createTextNode(contents))
        elem.appendChild(failEl)
        time_taken = 0
        elem.setAttribute('time',str(time_taken))
        elem.setAttribute('totalTime',str(time_taken))
        self._doc.documentElement.appendChild(elem)
    
    def addSuccessTestCase(self, algorithm):
        elem = self._doc.createElement('testcase')
        elem.setAttribute('classname', 'WikiMaker')
        elem.setAttribute('name', algorithm)
        time_taken = 0
        elem.setAttribute('time',str(time_taken))
        elem.setAttribute('totalTime',str(time_taken))
        self._doc.documentElement.appendChild(elem)
    
    
    def addFailureTestCase(self, algorithm, version, last_editor, diff):
        contents = "Algorithm %s Version %i last edited by %s is out of sync.\n\nDifferences are:\n\n%s" % (algorithm, version, last_editor, diff)
        self.__addGenericFailure__(contents, algorithm)
        
    def addFailureNoDescription(self, algorithm, version):
        contents = "Algorithm %s Version %i has no description" % (algorithm, version)
        self.__addGenericFailure__(contents, algorithm)

