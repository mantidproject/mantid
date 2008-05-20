import re
import smtplib
import os
from shutil import move
from time import strftime

#Email settings
smtpserver = 'outbox.rl.ac.uk'
localServerName = 'http://130.246.49.183/'
#localServerName = 'file://c|/Program Files/CruiseControl/'

#RECIPIENTS = ['r.tolchenov@rl.ac.uk']
RECIPIENTS = ['mantid-buildserver@mantidproject.org']
#,'mantid-developers@mantidproject.org'
SENDER = 'BuildServer1@mantidproject.org'
if (os.name =='nt'):
     SENDER = 'Win' + SENDER
else:
     SENDER = 'Linux' + SENDER

#Set up email content 
buildSuccess = False
testsBuildSuccess = False
sconsResult = ''
testsResult = ''
testsPass = True

mssgScons = ''
mssgSconsErr = ''
mssgTestsBuild = ''
mssgTestsErr = ''
mssgTestsRunErr = ''
mssgTestsResults = ''
mssgSvn  = ''
mssgDoxy = ''
logDir = '../../../../logs/Mantid/'

testCount = 0
failCount = 0

#create archive directory
archiveDir = logDir + strftime("%Y-%m-%d_%H-%M-%S")
os.mkdir(archiveDir)


#Get scons result and errors
fileScons = logDir+'scons.log'
f = open(fileScons,'r')

for line in f.readlines():
     sconsResult = line
     mssgScons = mssgScons + line
     
f.close()
move(fileScons,archiveDir)

if sconsResult.startswith('scons: done building targets.'):
	buildSuccess = True	
	
fileSconsErr = logDir+'sconsErr.log'
mssgSconsErr = open(fileSconsErr,'r').read()
move(fileSconsErr,archiveDir)

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
f = open(filetestsRunErr,'r')

for line in f.readlines():
	temp = line
	if temp.startswith('TestsScript.sh:'):
		testsBuildSuccess = False
		mssgTestsRunErr  = mssgTestsRunErr + temp[0:temp.find('>>')] + '\n'
     
f.close()
move(filetestsRunErr,archiveDir)

#Get tests result
filetestsRun = logDir+'testResults.log'
f = open(filetestsRun,'r')

reTestCount = re.compile("Running\\s*(\\d+)\\s*tests", re.IGNORECASE)
reFailCount = re.compile("Failed\\s*(\\d+)\\s*of\\s*(\\d+)\\s*tests", re.IGNORECASE)
for line in f.readlines():
	m=reTestCount.match(line)
        if m:
            testCount += int(m.group(1))
        m=reFailCount.match(line)
        if m:
            failCount += int(m.group(1))
            testsPass = False
	mssgTestsResults = mssgTestsResults + line
     
f.close()
move(filetestsRun,archiveDir)

#Read svn log
filesvn = logDir+'svn.log'
mssgSvn = open(filesvn,'r').read()
move(filesvn,archiveDir)

#Read doxygen log
filedoxy = logDir+'doxy.log'
mssgDoxy = open(filedoxy,'r').read()
move(filedoxy,archiveDir)

#Construct Message
httpLinkToArchive = localServerName + archiveDir.replace('../../../../','') + '/'
message = 'Build Completed at: ' + strftime("%H:%M:%S %d-%m-%Y") + "\n"
message += 'Framework Build Passed: ' + str(buildSuccess) + "\n"
message += 'Tests Build Passed: ' + str(testsBuildSuccess) + "\n"
message += 'Units Tests Passed: ' + str(testsPass) 
message += ' (' 
if failCount>0:
    message += str(failCount) + " failed out of "
message += str(testCount) + " tests)\n\n"
message += mssgSvn + "\n"
message += 'FRAMEWORK BUILD LOG\n\n'
message += 'Build stdout <' + httpLinkToArchive + 'scons.log>\n'
message += 'Build stderr <' + httpLinkToArchive + 'sconsErr.log>\n'
#message += mssgScons + "\n\n"
#message += mssgSconsErr + "\n"
message += '------------------------------------------------------------------------\n'
message += 'TESTS BUILD LOG\n\n'
message += 'Test Build stdout <' + httpLinkToArchive + 'testsBuild.log>\n'
message += 'Test Build stderr <' + httpLinkToArchive + 'testsBuildErr.log>\n'
#message += mssgTestsBuild + "\n\n"
#message += mssgTestsErr + "\n"
#message += mssgTestsRunErr  + "\n"
message += '------------------------------------------------------------------------\n'
message += 'UNIT TEST LOG\n\n'
message += 'Test Run stderr <' + httpLinkToArchive + 'testResults.log>\n'
message += 'Test Run stderr <' + httpLinkToArchive + 'testsRunErr.log>\n'
#message += mssgTestsResults + "\n"
message += '------------------------------------------------------------------------\n'
message += 'DOXYGEN LOG\n\n'
message += 'Doxygen Log <' + httpLinkToArchive + 'doxy.log>\n'
#message += mssgDoxy + "\n"

#Create Subject
subject = 'Subject: '
if (os.name=='nt'):
     subject += "Windows"
else:
     subject += "Linux"
          
subject += ' Build Report: '

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

#Send Email
session = smtplib.SMTP(smtpserver)

smtpresult  = session.sendmail(SENDER, RECIPIENTS, subject  + message)

if smtpresult:
    errstr = ""
    for recip in smtpresult.keys():
        errstr = """Could not delivery mail to: %s

Server said: %s
%s

%s""" % (recip, smtpresult[recip][0], smtpresult[recip][1], errstr)
    raise smtplib.SMTPException, errstr

if not buildSuccess:
     exit(1)
else:
     fileLaunchQTIPlot = logDir+'LaunchQTIPlot.txt'
     f = open(fileLaunchQTIPlot,'w')
     f.write('launch')

