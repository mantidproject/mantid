#!/usr/bin/env python
ROOT_URL = "http://download.mantidproject.org/logs/"
SCONS_SUCCESS = "scons: done building targets."
MANTID = "Mantid"
QTIPLOT = "qtiplot"
LOGDIRS = [MANTID, QTIPLOT]
COLOR = {"green":'\x1b[32m', # copied from ../MantidBuild.py
         "blue":'\x1b[34m',
         "red":'\x1b[31m',
         "orange":'\x1b[33m',
         "reset":'\x1b[0m',
         "boldred":'\x1b[91m'
         }
OK = COLOR["green"] + "OK" + COLOR["reset"]
FAIL = COLOR["boldred"] + "FAILED" + COLOR["reset"]

import urllib2
from sendEmail import ParseSubversionString, ParseSconsString, ParseTestBuildString, ParseTestResultsString, ParseQtiBuildString

def getHtml(url, verbose=False):
    website = urllib2.urlopen(url)
    return website.read()

def getDir(text):
    opentag = "<a href="
    if opentag in text:
        start = text.index(opentag) + len(opentag) + 1 # past the open quote
    else:
        raise RuntimeError("Failed to find %s" % opentag)
    stop = text.index('>', start)
    text = text[start:stop-1]
    text = text.replace("/", "")
    return text

def getListing(url, verbose = 0):
    # read the table from the web page
    data = getHtml(url)
    if verbose:
        print data

    # get just the table
    start = data.find("<table>")
    stop = data.find("</table>", start)
    data = data[start + len("<table>"):stop]
    if verbose > 1:
        print "--------------------"
        print data

    # split the table into rows
    data = data.split("<tr>")
    data = [item for item in data if len(item) > 0]
    data = [item for item in data if not "<th" in item]
    data = [item for item in data if not "Parent Directory" in item]
    if verbose > 1:
        print "--------------------"
        print data


    # turn this into directories
    return [getDir(item) for item in data]

def toISO8601(datetime):
    (date, time) = datetime.split("_")
    time = time.replace("-", ":")
    return "%sT%sZ" % (date, time)

def checkScons(url):
    data = getHtml(url)
    return SCONS_SUCCESS in data

def checkTests(url):
    data = getHtml(url)
    return None # TODO find a way to check

def getMostRecent(url):
    dates = getListing(url)
    url += '/' + dates[-1]
    return (toISO8601(dates[-1]), url, getListing(url))

def printResultHeader(label, url):
    dates = getListing(url)
    date = dates[-1]
    print label, toISO8601(date)

def printMantidResult(url, verbose=0):
    (date, url, files) = getMostRecent(url)
    results = {}
    all_good = True
    for log in ["scons.log", "testsBuild.log"]:
        if log in files:
            if checkScons(url + '/' + log):
                results[log] = "ok"
            else:
                results[log] = "failed"
                all_good = False
        else:
            results[log]="missing"
            all_good = False
        #print log, results[log]
    results["testResults.log"] = "UNKNOWN"
    # SVN RESULT
    (tickets, revision, stuff) = ParseSubversionString(getHtml(url + '/svn.log'))
    revision = str(revision)
    tickets = [("#%s" % str(ticket)) for ticket in tickets]
    tickets = ", ".join(tickets)

    # SCONS RESULT
    (sconsBuild, sconsWarn, stuff) \
        = ParseSconsString(getHtml(url + '/scons.log'),
                           getHtml(url + '/sconsErr.log'))
    # SCONS TEST RESULT
    (testBuild, testBuildWarn, stuff) \
        = ParseSconsString(getHtml(url + '/testsBuild.log'),
                           getHtml(url + '/testsBuildErr.log'))
    # TEST RUNNING RESULT
    (testCount, testFailCount, testRun, stuff) \
        = ParseTestResultsString(getHtml(url + '/testResults.log'))

    # print out the summary
    if verbose:
        print "r%s addressing %s" % (str(revision), tickets)
    if sconsBuild and testBuild and testRun:
        print url, OK
        if not verbose:
            return
    else:
        print url, FAIL
    # something went wrong so give more details
    print "scons build", sconsWarn, "warnings build",
    if sconsBuild:
        print OK
    else:
        print FAIL
    print "scons test build", testBuildWarn, "warnings build",
    if testBuild:
        print OK
    else:
        print FAIL
    print "%d of %d passed" % (testRunResult[0] - testRunResult[1], testRunResult[0])

def printQtiResult(url, verbose=0):
    (date, url, files) = getMostRecent(url)
    results = {}
    all_good = True

    # do the checks
    (build, warn) = ParseQtiBuildString(getHtml(url + '/build.log'),
                                        getHtml(url + '/error.log'))

    # print out the summary
    print url,
    if build:
        print OK,
    else:
        print FAIL,
    if warn > 0:
        print "[%d warnings]" % warn
    else:
        print

if __name__ == "__main__":
    platforms = getListing(ROOT_URL)
    for platform in platforms:
        print "*******************************", platform, "results"
        subdirs = getListing(ROOT_URL + platform)
        subdirs = [direc for direc in subdirs if direc in LOGDIRS]
        if MANTID in subdirs:
            printMantidResult(ROOT_URL + platform + '/' + MANTID)
        if QTIPLOT in subdirs:
            printQtiResult(ROOT_URL + platform + '/' + QTIPLOT)
