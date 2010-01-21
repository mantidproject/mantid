import re
import smtplib
import os
import sys
import platform
from shutil import move
from time import strftime

#Email settings
smtpserver = 'outbox.rl.ac.uk'
#RECIPIENTS = ['martyn.gigg@stfc.ac.uk']
RECIPIENTS = ['mantid-buildserver@mantidproject.org']
SENDER = platform.system() + 'BuildServer1@mantidproject.org'
localServerName = 'http://' + platform.node()
if platform.system() == 'Darwin':
     localServerName += ':8080'
localServerName += '/'

#Set up email content 
buildSuccess = True

logDir = '../../../../logs/qtiplot/'

#create archive directory
archiveDir = logDir + strftime("%Y-%m-%d_%H-%M-%S")
os.mkdir(archiveDir)

#Get build result and errors
fileBuild = logDir+'build.log'
f = open(fileBuild,'r')

for line in f:
     buildResult = line
     
f.close()
move(fileBuild,archiveDir)

if 'failed' in buildResult:
     buildSuccess = False	
	
fileBuildErr = logDir+'error.log'
move(fileBuildErr,archiveDir)

if buildSuccess:
     fileLaunchInstaller = logDir+'LaunchInstaller.txt'
     f = open(fileLaunchInstaller,'w')
     f.write('launch')

last = logDir + 'lastBuild.txt'
try:
     lastBuild = open(last,'r').read()
except IOError:
     lastBuild = 'False'
open(last,'w').write(str(buildSuccess))

# We want to skip sending email if this AND previous build succeeded
if buildSuccess and lastBuild=='True':
     exit(0)

#Construct Message
httpLinkToArchive = localServerName + archiveDir.replace('../../../../','') + '/'
message = 'Build Completed at: ' + strftime("%H:%M:%S %d-%m-%Y") + "\n"
message += 'MantidPlot Build Passed: ' + str(buildSuccess) + "\n\n"
message += '-----------------------------------------------------------------------\n'
message += 'QTIPLOT BUILD LOG\n\n'
message += 'Build stdout <' + httpLinkToArchive + 'build.log>\n'
message += 'Build stderr <' + httpLinkToArchive + 'error.log>\n'

#Create Subject
subject = 'Subject: ' + platform.system() + ' Build Report: '

if buildSuccess:
	subject += '[MantidPlot Build Successful]\n\n\n'
else:
	subject += '[MantidPlot Build Failed]\n\n\n'
	
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
    raise smtplib.SMTPException, errstr

if buildSuccess:
     sys.exit(0)
else:
     sys.exit(1)
