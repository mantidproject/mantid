import platform
import socket
import os
import re
import urllib2
import smtplib
from time import strftime
import subprocess as sp
import shutil
import buildNotification
import sys

SendEmail = bool(False)
OverAllSuccess = bool(False)

base_url = "http://mantidlx1/~tzh47418/"
trac_link = 'http://trac.mantidproject.org/mantid/'

project = 'Mantid'
localLogDir = '../../../../logs/' + project + '/'

def ParseSubversionLog():
    ticketList = []
    SvnID = -1
    mssgSvn = ''
    filesvn = localLogDir + 'svn.log'
    mssgSvn = open(filesvn,'r').read()
    buildNotification.moveToArchive(filesvn,remoteArchivePath)
    #attempt to parse out the svn revision and ticket number
    reSvnRevision = re.compile("r(\\d+)\\s\\|", re.IGNORECASE)
    m=reSvnRevision.search(mssgSvn)
    if m:
        SvnID = m.group(1)
    reTicket = re.compile("#(\\d+)", re.IGNORECASE)
    mList=reTicket.findall(mssgSvn)
    for m in mList:
        ticketList.append(m)
    return ticketList, SvnID, mssgSvn
#end of ParseSubversionLog

def ParseSconsLog():
    mssgSconsWarn = ''
    compilerWarnCount = 0
    FrameWorkBuild = bool(False)
    fileScons = localLogDir + 'scons.log'
    f = open(fileScons, 'r')
    for line in f.readlines():
        sconsResult = line
        if os.name == 'nt':
            mssgSconsWarn = mssgSconsWarn + line
    f.close()
    buildNotification.moveToArchive(fileScons, remoteArchivePath)
    if sconsResult.startswith('scons: done building targets.'):
        FrameWorkBuild = True
    # error file for Windows/Mac
    fileSconsErr = localLogDir + 'sconsErr.log'
    if os.name == 'posix':
        mssgSconsWarn = open(fileSconsErr, 'r').read()
    buildNotification.moveToArchive(fileSconsErr, remoteArchivePath)
	#count compilation warnings
    reWarnCount = re.compile(": warning")
    wc = reWarnCount.findall(mssgSconsWarn)
    compilerWarnCount = len(wc)
    return mssgSconsWarn, compilerWarnCount, FrameWorkBuild
#end of ParseSconsLog

def ParseTestBuildLog():
    TestsBuild = bool(False)
    mssgTestsBuild = ''
    mssgTestsErr = ''
    fileTestsBuild = localLogDir + 'testsBuild.log'
    f = open(fileTestsBuild, 'r')
    for line in f.readlines():
        testsResult = line
        mssgTestsBuild = mssgTestsBuild + line
    f.close()
    buildNotification.moveToArchive(fileTestsBuild, remoteArchivePath)
    if testsResult.startswith('scons: done building targets.'):
        TestsBuild = True
    fileTestsBuildErr = localLogDir + 'testsBuildErr.log'
    mssgTestsErr = open(fileTestsBuildErr, 'r').read()
    buildNotification.moveToArchive(fileTestsBuildErr, remoteArchivePath)
    fileTestsRunErr = localLogDir + 'testsRunErr.log'
    buildNotification.moveToArchive(fileTestsRunErr, remoteArchivePath)
    return TestsBuild, mssgTestsBuild, mssgTestsErr
#end of ParseTestBuildLog

def ParseTestResultsLog():
    testCount = 0
    failCount = 0
    UnitTests = bool(True)
    mssgTestsResults = ''
    fileTestsRun = localLogDir + 'testResults.log'
    f = open(fileTestsRun, 'r')
    reTestCount = re.compile("Running\\s*(\\d+)\\s*test", re.IGNORECASE)
    reCrashCount = re.compile("OK!")
    reFailCount = re.compile("Failed\\s*(\\d+)\\s*of\\s*(\\d+)\\s*tests", re.IGNORECASE)
    reFailSuite = re.compile("A fatal error occurred", re.IGNORECASE)
    for line in f.readlines():
        m = reTestCount.search(line)
        if m:
            testCount += int(m.group(1))
            m = reCrashCount.search(line)
            if not m:
                failCount += 1
                UnitTests = False
        m = reFailCount.match(line)
        if m:
            failCount -= 1
            failCount += int(m.group(1))
            UnitTests = False
        m = reFailSuite.match(line)
        if m:
            UnitTests = False
            failCount += 1
        mssgTestsResults = mssgTestsResults + line
    f.close()
    buildNotification.moveToArchive(fileTestsRun, remoteArchivePath)
    return testCount, failCount, UnitTests, mssgTestsResults
#end of ParseTestResultsLog

def RecordExists():
    url = base_url + "rec_exists.psp?id=" + str(SvnID)
    f = urllib2.urlopen(url)
    page = f.read()
    if re.search("Success", page):
        r = True
    elif re.search("Failure", page):
        r = False
    else:
        r = False #needs something better here, but it *should* be controlled by the db
    return r
#end of RecordExists

def CreateRecord():
    url = base_url + "rec_create.psp?id=" + str(SvnID)
    f = urllib2.urlopen(url)
#end of CreateRecord()

def GetPlatformID():
    r = os.name + platform.system() + platform.architecture()[0]
    return r
#end of GetPlatformID()

def CreateTREntry():
    url = base_url + "tr.psp?id=" + str(SvnID)
    url += "&plat=" + GetPlatformID()
    url += "&fw=" + str(FrameWorkBuild)
    url += "&tb=" + str(TestsBuild)
    url += "&ut=" + str(UnitTests)
    if os.name != 'posix':
        url += "&dx=" + str(DocumentationBuild)
    url += "&success=" + str(OverAllSuccess)
    url += "&cw=" + str(compilerWarnCount)
    url += "&t=" + str(testCount)
    url += "&tf=" + str(failCount)
    url += "&dw=" + str(warnCount)
    url += "&utt=" + str(UnitTestsTime)
    url += "&btt=" + str(TestBuildTime)
    url += "&bdt=" + str(SconBuildTime)
    f = urllib2.urlopen(url)
#end of CreateTREntry(SvnID)

def CreateTRACRecord():
    global ticketList
    url = base_url + "trac.psp?id=" + str(SvnID) + "&trac="
    for i in ticketList:
        f = urllib2.urlopen(url + str(i))
# end CreateTRACRecord

def GetTimes():
    SconBuildTime = 0.0
    TestBuildTime = 0.0
    UnitTestsTime = 0.0
    fileBuildTime = localLogDir + 'timebuild.log'
    fileBuildTest = localLogDir + 'timetestbuild.log'
    fileRunTests = localLogDir + 'timetestrun.log'
    f = open(fileBuildTime, 'r')
    SconBuildTime = float(f.read())
    f.close()
    f = open(fileBuildTest, 'r')
    TestBuildTime = float(f.read())
    f.close()
    f = open(fileRunTests, 'r')
    UnitTestsTime = float(f.read())
    f.close()
    buildNotification.moveToArchive(fileBuildTime, remoteArchivePath)
    buildNotification.moveToArchive(fileBuildTest, remoteArchivePath)
    buildNotification.moveToArchive(fileRunTests, remoteArchivePath)
    return SconBuildTime, TestBuildTime, UnitTestsTime
#end of GetTimes()

def ParseDoxygenLog():
    warnCount = 0
    DocumentationBuild = bool(True)
    mssgDoxy = 0
    lastDoxy = 0
    filedoxy = localLogDir + 'doxy.log'
    mssgDoxy = open(filedoxy, 'r').read()
    reWarnCount = re.compile("Warning: ")
    m = reWarnCount.findall(mssgDoxy)
    if m:
        warnCount = len(m)
        if warnCount > 0:
            DocumentationBuild = False
    buildNotification.moveToArchive(filedoxy, remoteArchivePath)
    try:
        lastDoxy = int(open('prevDoxy', 'r').read())
    except IOError:
        lastDoxy = 0
    currentDoxy = open('prevDoxy', 'w')
    currentDoxy.write(str(warnCount))
    return warnCount, DocumentationBuild, mssgDoxy, lastDoxy
#end of ParseDoxygenLog

def SendNotificationEmail():
    SENDER = platform.system() 
    #Create Subject
    subject = 'Subject: ' + platform.system() 
    if platform.architecture()[0] == '64bit':
        SENDER += '64'
        subject += '64'
    SENDER += 'BuildServer1@mantidproject.org'
    subject += ' Build Report: '

    httpLinkToArchive = 'http://download.mantidproject.org/' + relativeLogDir.replace("\\","/")

    #compose message
    message = "Build Completed at: " + strftime("%H:%M:%S %d-%m-%Y") + "\n"
    message += "Framework Build Passed: " + str(FrameWorkBuild) + " (" + str(compilerWarnCount) + " compiler warnings)" + "\n"
    message += "Tests Build Passed: " + str(TestsBuild) + "\n"
    message += "Unit Tests Passed: " + str(UnitTests) + " ( " + str(failCount) + " failed out of " + str(testCount) + " tests.)" + "\n"
    if os.name != 'posix': # doxy stuff here
        message += "Code Documentation Passed: " + str(DocumentationBuild)
        if warnCount > 0:
            message += " ("+str(warnCount) +" doxygen warnings"
            if (warnCount > lastDoxy):
                message += " - " + str(warnCount-lastDoxy) + " MORE FROM THIS CHECK-IN"
            if (lastDoxy > warnCount):
                message += " - " + str(lastDoxy-warnCount) + " fewer due to this check-in"  
            message += ")\n"
        else:
            message += "\n"
    message += "\n"

    message += "SVN Revision: " + str(SvnID)
    message += " " + trac_link + "changeset/" + str(SvnID) + "\n"
    for ticket in ticketList:
        message += "TRAC Ticket: " + ticket
        message += " " + trac_link + "ticket/"  + ticket + "\n"

    message += mssgSvn + "\n"
    message += 'FRAMEWORK BUILD LOG\n\n'
    message += 'Build stdout <' + httpLinkToArchive + 'scons.log>\n'
    message += 'Build stderr <' + httpLinkToArchive + 'sconsErr.log>\n'
    message += '------------------------------------------------------------------------\n'
    message += 'TESTS BUILD LOG\n\n'
    message += 'Test Build stdout <' + httpLinkToArchive + 'testsBuild.log>\n'
    message += 'Test Build stderr <' + httpLinkToArchive + 'testsBuildErr.log>\n'
    message += '------------------------------------------------------------------------\n'
    message += 'UNIT TEST LOG\n\n'
    message += 'Test Run stdout <' + httpLinkToArchive + 'testResults.log>\n'
    message += 'Test Run stderr <' + httpLinkToArchive + 'testsRunErr.log>\n'
    message += '------------------------------------------------------------------------\n'
    # Not doing doxygen on the Mac
    if os.name != 'posix':
        message += 'DOXYGEN LOG\n\n'
        message += 'Doxygen Log <' + httpLinkToArchive + 'doxy.log>\n'
    if FrameWorkBuild:
	    subject += '[Framework Build Successful, '
    else:
	    subject += '[Framework Build Failed, '
    if TestsBuild:
	    subject += 'Tests Build Successful, '
    else:
	    subject += 'Tests Build Failed, '
    if UnitTests:
	    subject += 'Tests Successful]\n'
    else:
	    subject += 'Tests Failed]\n'
    logfile = buildNotification.sendResultMail(subject+message,localLogDir,sender=SENDER)
# The program logic starts here.

remoteArchivePath, relativeLogDir = buildNotification.getArchiveDir(project)

ticketList, SvnID, mssgSvn = ParseSubversionLog()

if not RecordExists():
    SendEmail = True
    CreateRecord()
    CreateTRACRecord()

mssgSconsWarn, compilerWarnCount, FrameWorkBuild = ParseSconsLog()
TestsBuild, mssgTestsBuild, mssgTestsErr = ParseTestBuildLog()
testCount, failCount, UnitTests, mssgTestsResults = ParseTestResultsLog()
SconBuildTime, TestBuildTime, UnitTestsTime = GetTimes()

if os.name != 'posix':
    warnCount, DocumentationBuild, mssgDoxy, lastDoxy = ParseDoxygenLog()

if FrameWorkBuild and TestsBuild and UnitTests:
    OverAllSuccess = True
else:
    SendEmail = True

CreateTREntry()

if SendEmail:
    SendNotificationEmail()

if not OverAllSuccess:
     # On Redhat we have python 2.4 and that doesn't seem to have the global exit command
     sys.exit(1)
else:
     fileLaunchQTIPlot = localLogDir+'LaunchQTIPlot.txt'
     f = open(fileLaunchQTIPlot,'w')
     f.write('launch')