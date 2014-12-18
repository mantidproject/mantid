import os
import sys

#########################################################################
# A base class to support report results in an appropriate manner
#########################################################################
class ResultReporter(object):
    '''
    A base class for results reporting. In order to get the results in an
    appropriate form, subclass this class and implement the dispatchResults
    method.
    '''

    def __init__(self):
        '''Initialize a class instance, e.g. connect to a database'''
        pass

    def dispatchResults(self, result):
        """
        Parameters
            result: a TestResult object """
        raise NotImplementedError('"dispatchResults(self, result)" should be overridden in a derived class')


#########################################################################
# A class to report results as formatted text output
#########################################################################
class TextResultReporter(ResultReporter):
    '''
    Report the results of a test using standard out
    '''

    def dispatchResults(self, result):
        '''
        Print the results to standard out
        '''
        nstars = 30
        print '*' * nstars
        for (name, val) in result.data.items():
            str_val = str(val)
            str_val = str_val.replace("\n", " ")
            if len(str_val) > 50:
                str_val = str_val[:50] + " . . . "
            print '    ' + name.ljust(15) + '->  ', str_val
        print '*' * nstars


#########################################################################
# A class to report results as formatted text output
#########################################################################
class LogArchivingReporter(ResultReporter):
    '''
    Report the results of a test using standard out
    '''
    def __init__(self, logarchive):
        # Path to a log archiving folder
        self.logarchive = os.path.abspath(logarchive)
        if not os.path.exists(self.logarchive):
            os.mkdir(self.logarchive)

    def dispatchResults(self, result):
        '''
        Print the results to standard out
        '''
        fullpath = os.path.join(self.logarchive, result.get_logarchive_filename())
        f = open(fullpath, "w")
        f.write(result["log_contents"])
        f.close()

#########################################################################
# A class to report results as XML that Hudson can interpret
#########################################################################
class JUnitXMLReporter(ResultReporter):
    '''
    Report the results of a test to a JUnit style XML format
    that can be read by Hudson/Jenkins
    '''

    def __init__(self, path):
        # Path to .xml files
        self._path = path

    def dispatchResults(self, result):
        '''
        Make a junit .xml file
        '''
        fullpath = os.path.join(self._path, "%s.xml" % result["name"])
        f = open(fullpath, 'w')

        names  = result["name"].split(".")
        suitename = names[0]
        testname = ".".join(names[1:])

        failure = ""
        num_failures = 0
        if not result["success"]:
            failure = """\n        <failure type="failedAssert">%s</failure>
            <system-out ><![CDATA[%s]]></system-out>""" % (result["status"], result["log_contents"])
            num_failures = 1

        f.write("""<?xml version="1.0" encoding="UTF-8"?>
<testsuite name="%s" tests="1" failures="%d" disabled="0" errors="0" time="0.0">
    <testcase name="%s" time="%f" classname="%s">%s
    </testcase>
</testsuite>
""" % (suitename, num_failures, testname, result["runtime"], suitename, failure) )



if __name__=="__main__":
    import testresult
    rep = JUnitXMLReporter(".")

    res = testresult.TestResult()
    res["name"] = "MyTestTest.Test"
    res["status"] = "success maybe?"
    res["success"] = True
    res["runtime"] = 1.234
    rep.dispatchResults(res)

    res = testresult.TestResult()
    res["name"] = "MyTestTest.OtherTest"
    res["status"] = "failure"
    res["success"] = False
    res["runtime"] = 3.456
    rep.dispatchResults(res)
