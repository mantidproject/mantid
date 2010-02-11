import re
import smtplib
import socket
import os
import sys
import platform
import subprocess as sp
from shutil import move
sys.path.insert(0,'../Mantid/Build')
import buildNotification as notifier
from time import strftime

#Email settings
smtpserver = 'outbox.rl.ac.uk'
#RECIPIENTS = ['martyn.gigg@stfc.ac.uk']
RECIPIENTS = ['mantid-buildserver@mantidproject.org']
SENDER = platform.system() + 'BuildServer1@mantidproject.org'

#Set up email content 
buildSuccess = True

project = 'qtiplot'
remoteArchiveDir, relativeLogDir = notifier.getArchiveDir(project)
localBaseLog = '../../../../logs/' 
localLogDir = localBaseLog + project + '/'

#Get build result and errors
fileBuild = localLogDir+'build.log'
f = open(fileBuild,'r')

for line in f:
     buildResult = line
     
f.close()
notifier.moveToArchive(fileBuild,remoteArchiveDir)

if 'failed' in buildResult:
     buildSuccess = False	
	
fileBuildErr = localLogDir+'error.log'
notifier.moveToArchive(fileBuildErr,remoteArchiveDir)

mtdtests = localBaseLog + 'Mantid' + '/MantidTests.txt'
f = open(mtdtests, 'r')
for line in f:
     mtdTestsResult = line
f.close()
if 'False' in mtdTestsResult:
     mtdTestsPassed = False
else:
     mtdTestsPassed = True

if buildSuccess and mtdTestsPassed:
     fileLaunchInstaller = localLogDir + 'LaunchInstaller.txt'
     f = open(fileLaunchInstaller,'w')
     f.write('launch')

last = localLogDir + 'lastBuild.txt'
try:
     lastBuild = open(last,'r').read()
except IOError:
     lastBuild = 'False'
open(last,'w').write(str(buildSuccess))

# We want to skip sending email if this AND previous build succeeded
if buildSuccess and lastBuild=='True':
     sys.exit(0)

#Construct Message
httpLinkToArchive = 'http://download.mantidproject.org/' + relativeLogDir.replace("\\","/")

message = 'Build Completed at: ' + strftime("%H:%M:%S %d-%m-%Y") + "\n"
message += 'MantidPlot Build Passed: ' + str(buildSuccess) + "\n\n"
message += '-----------------------------------------------------------------------\n'
message += 'MANTIDPLOT BUILD LOG\n\n'
message += 'Build stdout <' + httpLinkToArchive + 'build.log>\n'
message += 'Build stderr <' + httpLinkToArchive + 'error.log>\n'

#Create Subject
subject = 'Subject: ' + platform.system() + ' Build Report: '

if buildSuccess:
	subject += '[MantidPlot Build Successful]\n\n\n'
else:
	subject += '[MantidPlot Build Failed]\n\n\n'


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

if buildSuccess:
     sys.exit(0)
else:
     sys.exit(1)
