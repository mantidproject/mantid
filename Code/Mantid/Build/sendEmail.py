import re
import smtplib
import os
import platform
import socket
from shutil import move
from time import strftime
import buildNotification

#Email settings
smtpserver = 'outbox.rl.ac.uk'

#RECIPIENTS = ['russell.taylor@stfc.ac.uk']
RECIPIENTS = ['mantid-buildserver@mantidproject.org']
SENDER = platform.system()+'BuildServer1@mantidproject.org'

localServerName = 'http://' + platform.node()
if platform.system() == 'Darwin':
     localServerName += ':8080'
localServerName += '/'

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
logDir = '../../../../logs/Mantid/'

testCount = 0
failCount = 0
warnCount = 0

#create archive directory
archiveDir = logDir + strftime("%Y-%m-%d_%H-%M-%S")
os.mkdir(archiveDir)


#Get scons result and errors
fileScons = logDir+'scons.log'
f = open(fileScons,'r')

for line in f.readlines():
     sconsResult = line
     if os.name == 'nt':
          mssgSconsWarn = mssgSconsWarn + line
     
f.close()
move(fileScons,archiveDir)

if sconsResult.startswith('scons: done building targets.'):
	buildSuccess = True	

# On Linux/Mac, compilation warning are in stderr file.
# On Windows they're in stdout (read above)
fileSconsErr = logDir+'sconsErr.log'
if os.name == 'posix':
     mssgSconsWarn = open(fileSconsErr,'r').read()
move(fileSconsErr,archiveDir)

# Count compilation warnings
reWarnCount = re.compile(": warning")
wc = reWarnCount.findall(mssgSconsWarn)
compilerWarnCount = len(wc)

#Get tests scons result and errors
filetestsBuild = logDir+'testsBuild.log'
f = open(filetestsBuild,'r')

for line in f.readlines():
     testsResult = line
     mssgTestsBuild = mssgTestsBuild + line
     
f.close()
move(filetestsBuild,archiveDir)

if testsResult.startswith('scons: done building targets.'):
	testsBuildSuccess = True	
	
filetestsBuildErr = logDir+'testsBuildErr.log'
mssgTestsErr = open(filetestsBuildErr,'r').read()
move(filetestsBuildErr,archiveDir)

filetestsRunErr = logDir+'testsRunErr.log'
move(filetestsRunErr,archiveDir)

#Get tests result
filetestsRun = logDir+'testResults.log'
f = open(filetestsRun,'r')

reTestCount = re.compile("Running\\s*(\\d+)\\s*test", re.IGNORECASE)
reCrashCount = re.compile("OK!")
reFailCount = re.compile("Failed\\s*(\\d+)\\s*of\\s*(\\d+)\\s*tests", re.IGNORECASE)
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
        mssgTestsResults = mssgTestsResults + line
     
f.close()
move(filetestsRun,archiveDir)

#Read svn log
filesvn = logDir+'svn.log'
mssgSvn = open(filesvn,'r').read()
move(filesvn,archiveDir)
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
if platform.system() != 'Darwin':
     filedoxy = logDir+'doxy.log'
     mssgDoxy = open(filedoxy,'r').read()
     reWarnCount = re.compile("Warning:")
     m=reWarnCount.findall(mssgDoxy)
     if m:
          warnCount = len(m)
          if warnCount >0:
               doxyWarnings = False
     move(filedoxy,archiveDir)

     lastDoxy = int(open('prevDoxy','r').read())
     currentDoxy = open('prevDoxy','w')
     currentDoxy.write(str(warnCount))

#Notify build completion
buildErrors =1
testBuildErrors=1
if buildSuccess:
  buildErrors=0
if testsBuildSuccess:
  testBuildErrors=0
buildNotification.sendTestCompleted(project="Mantid", \
                    testCount=testCount,testFail=failCount, \
                    compWarn=compilerWarnCount,docuWarn=warnCount, \
                    buildErrors=buildErrors,testBuildErrors=testBuildErrors)

#Construct Message
httpLinkToArchive = localServerName + archiveDir.replace('../../../../','') + '/'
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
if platform.system() != 'Darwin':
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
if platform.system() != 'Darwin':
     message += 'DOXYGEN LOG\n\n'
     message += 'Doxygen Log <' + httpLinkToArchive + 'doxy.log>\n'


#Create Subject
subject = 'Subject: ' + platform.system() + ' Build Report: '

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
socket.setdefaulttimeout(180)
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
      print errstr
except:
  print "Failed to send the build results email"

if not buildSuccess:
     exit(1)
else:
     fileLaunchQTIPlot = logDir+'LaunchQTIPlot.txt'
     f = open(fileLaunchQTIPlot,'w')
     f.write('launch')

