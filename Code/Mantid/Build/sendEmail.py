import re
import smtplib
import os
import platform
import socket
import sys
from shutil import move
sys.path.append('Build')
import buildNotification as notifier
from time import strftime

#Email settings
smtpserver = 'outbox.rl.ac.uk'

#RECIPIENTS = ['russell.taylor@stfc.ac.uk']
#RECIPIENTS=['martyn.gigg@stfc.ac.uk']
RECIPIENTS = ['mantid-buildserver@mantidproject.org']
SENDER = platform.system() 
#Create Subject
subject = 'Subject: ' + platform.system() 
if platform.architecture()[0] == '64bit':
    SENDER += '64'
    subject += '64'
SENDER += 'BuildServer1@mantidproject.org'
subject += ' Build Report: '

tracLink = 'http://trac.mantidproject.org/mantid/'
#Set up email content 
buildSuccess = False
testsBuildSuccess = False
sconsResult = ''
testsResult = ''
testsPass = True
doxyWarnings = True

mssgSconsWarn = ''
mssgTestsBuild = ''
mssgTestsErr = ''
mssgTestsResults = ''
mssgSvn  = ''
mssgDoxy = ''
ticketList = []
svnRevision = ''

testCount = 0
failCount = 0
warnCount = 0

project = 'Mantid'
remoteArchivePath, relativeLogDir = notifier.getArchiveDir(project)
localLogDir = '../../../../logs/' + project + '/'

#Get scons result and errors
fileScons = localLogDir + 'scons.log'
f = open(fileScons,'r')

for line in f.readlines():
     sconsResult = line
     if os.name == 'nt':
          mssgSconsWarn = mssgSconsWarn + line
     
f.close()
notifier.moveToArchive(fileScons,remoteArchivePath)

if sconsResult.startswith('scons: done building targets.'):
	buildSuccess = True	

# On Linux/Mac, compilation warning are in stderr file.
# On Windows they're in stdout (read above)
fileSconsErr =  localLogDir + 'sconsErr.log'
if os.name == 'posix':
     mssgSconsWarn = open(fileSconsErr,'r').read()
notifier.moveToArchive(fileSconsErr,remoteArchivePath)

# Count compilation warnings
reWarnCount = re.compile(": warning")
wc = reWarnCount.findall(mssgSconsWarn)
compilerWarnCount = len(wc)

#Get tests scons result and errors
filetestsBuild =  localLogDir + 'testsBuild.log'
f = open(filetestsBuild,'r')

for line in f.readlines():
     testsResult = line
     mssgTestsBuild = mssgTestsBuild + line
     
f.close()
notifier.moveToArchive(filetestsBuild,remoteArchivePath)

if testsResult.startswith('scons: done building targets.'):
	testsBuildSuccess = True	
	
filetestsBuildErr =  localLogDir + "testsBuildErr.log"
mssgTestsErr = open(filetestsBuildErr,'r').read()
notifier.moveToArchive(filetestsBuildErr,remoteArchivePath)

filetestsRunErr =  localLogDir + 'testsRunErr.log'
notifier.moveToArchive(filetestsRunErr,remoteArchivePath)

#Get tests result
filetestsRun = localLogDir + 'testResults.log'
f = open(filetestsRun,'r')

reTestCount = re.compile("Running\\s*(\\d+)\\s*test", re.IGNORECASE)
reCrashCount = re.compile("OK!")
reFailCount = re.compile("Failed\\s*(\\d+)\\s*of\\s*(\\d+)\\s*tests", re.IGNORECASE)
reFailSuite = re.compile("A fatal error occurred", re.IGNORECASE)
for line in f.readlines():
        m=reTestCount.search(line)
        if m:
            testCount += int(m.group(1))
            m=reCrashCount.search(line)
            if not m:
                failCount += 1
                testsPass = False
        m=reFailCount.match(line)
        if m:
            # Need to decrement failCount because crashCount will have incremented it above
            failCount -= 1
            failCount += int(m.group(1))
            testsPass = False
        m=reFailSuite.match(line)
        if m:
            testsPass = False
            failCount += 1
        mssgTestsResults = mssgTestsResults + line
     
f.close()
notifier.moveToArchive(filetestsRun,remoteArchivePath)

#Read svn log
filesvn = localLogDir + 'svn.log'
mssgSvn = open(filesvn,'r').read()
notifier.moveToArchive(filesvn,remoteArchivePath)
#attempt to parse out the svn revision and ticket number
reSvnRevision = re.compile("r(\\d+)\\s\\|", re.IGNORECASE)
m=reSvnRevision.search(mssgSvn)
if m:
  svnRevision = m.group(1)
  
reTicket = re.compile("#(\\d+)", re.IGNORECASE)
mList=reTicket.findall(mssgSvn)
for m in mList:
  ticketList.append(m)
  
#Read doxygen log - skip on Mac
if os.name != 'posix':
     filedoxy = localLogDir + 'doxy.log'
     mssgDoxy = open(filedoxy,'r').read()
     reWarnCount = re.compile("Warning:")
     m=reWarnCount.findall(mssgDoxy)
     if m:
          warnCount = len(m)
          if warnCount >0:
               doxyWarnings = False
     notifier.moveToArchive(filedoxy,remoteArchivePath)

     try:
          lastDoxy = int(open('prevDoxy','r').read())
     except IOError:
          lastDoxy = 0
     currentDoxy = open('prevDoxy','w')
     currentDoxy.write(str(warnCount))

#Notify build completion
buildErrors=1
testBuildErrors=1
if buildSuccess:
     buildErrors=0
if testsBuildSuccess:
     testBuildErrors=0
notifier.sendTestCompleted(project,testCount=testCount,testFail=failCount, \
                           compWarn=compilerWarnCount,docuWarn=warnCount, \
                           buildErrors=buildErrors,testBuildErrors=testBuildErrors)

#Construct Message
httpLinkToArchive = 'http://download.mantidproject.org/' + relativeLogDir.replace("\\","/")

message = 'Build Completed at: ' + strftime("%H:%M:%S %d-%m-%Y") + "\n"
message += 'Framework Build Passed: ' + str(buildSuccess)
if compilerWarnCount>0:
  message += " (" + str(compilerWarnCount) + " compiler warnings)"
message += '\n'
message += 'Tests Build Passed: ' + str(testsBuildSuccess) + "\n"
message += 'Units Tests Passed: ' + str(testsPass) 
message += ' (' 
if failCount>0:
    message += str(failCount) + " failed out of "
message += str(testCount) + " tests)\n"
# Not doing doxygen on the Mac
if os.name != 'posix':
     message += "Code Documentation Passed: " + str(doxyWarnings)
     if warnCount>0:
          message += " ("+str(warnCount) +" doxygen warnings"
          if (warnCount > lastDoxy):
               message += " - " + str(warnCount-lastDoxy) + " MORE FROM THIS CHECK-IN"
          if (lastDoxy > warnCount):
               message += " - " + str(lastDoxy-warnCount) + " fewer due to this check-in"  
          message += ")\n"
     else:
          message += "\n"
message += "\n"
if len(svnRevision) > 0:
  message += "SVN Revision: " + svnRevision
  message += " " + tracLink + "changeset/" + svnRevision + "\n"
for ticket in ticketList:
  message += "TRAC ticket: " + ticket 
  message += " " + tracLink + "ticket/" + ticket + "\n"
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

if buildSuccess:
	subject += '[Framework Build Successful, '
else:
	subject += '[Framework Build Failed, '
	
if testsBuildSuccess:
	subject += 'Tests Build Successful, '
else:
	subject += 'Tests Build Failed, '
	
if testsPass:
	subject += 'Tests Successful]\n'
else:
	subject += 'Tests Failed]\n'	

#timeout in seconds
socket.setdefaulttimeout(120)
emailErr = open(localLogDir + 'email.log','w')
try:
     #Send Email
     session = smtplib.SMTP(smtpserver)
     smtpresult  = session.sendmail(SENDER, RECIPIENTS, subject  + message)
     
     if smtpresult:
          errstr = ""
          for recip in smtpresult.keys():
               errstr = """Could not deliver mail to: %s
               Server said: %s
               %s
               %s""" % (recip, smtpresult[recip][0], smtpresult[recip][1], errstr)
               emailErr.write(errstr)
except smtplib.SMTPException, details:
     emailErr.write(str(details) + '\n')
emailErr.close()

# Write out what happened with the tests. 
# This is for deciding whether to build installer - only do so if tests both built and passed
mantidtests = localLogDir + 'MantidTests.txt'
f = open(mantidtests,'w')
testsPass = testsBuildSuccess and testsPass
f.write('Tests ' + str(testsPass))
f.close()

if not buildSuccess:
     # On Redhat we have python 2.4 and that doesn't seem to have the global exit command
     sys.exit(1)
else:
     fileLaunchQTIPlot = localLogDir+'LaunchQTIPlot.txt'
     f = open(fileLaunchQTIPlot,'w')
     f.write('launch')

