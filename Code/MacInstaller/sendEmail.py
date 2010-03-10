import re
import smtplib
import os
from shutil import move
from time import strftime
import sys
sys.append('../Mantid/Build')
import buildNotification as notifier

#Email settings
smtpserver = 'outbox.rl.ac.uk'

RECIPIENTS = ['mantid-buildserver@mantidproject.org']
SENDER = 'MacInstaller@mantidproject.org'

#Set up email content 
buildSuccess = False

mssgBuild = ''
mssgBuildErr = ''

localLogDir = '../../../../logs/Installer/'

#create archive directory
remoteArchiveDir,relativeLogDir = notifier.getArchiveDir('Installer')

#Get build result and errors
fileBuild = localLogDir+'build.log'
f = open(fileBuild,'r')

for line in f.readlines():
     buildResult = line
     mssgBuild = mssgBuild + line
     
f.close()
notifier.moveToArchive(fileBuild,remoteArchiveDir)

if buildResult.startswith('Success!'):
	buildSuccess = True
	
fileBuildErr = localLogDir+'error.log'
mssgBuildErr = open(fileBuildErr,'r').read()
notifier.moveToArchive(fileBuildErr,remoteArchiveDir)

#if buildSuccess:
#     sys.exit(0)

#Construct Message
httpLinkToArchive = 'http://download.mantidproject.org/' + relativeLogDir.replace("\\","/")
message = 'Build Completed at: ' + strftime("%H:%M:%S %d-%m-%Y") + "\n"
message += 'Build Passed: ' + str(buildSuccess) + "\n"
message += 'BUILD LOG\n\n'
message += 'Build stdout <' + httpLinkToArchive + 'build.log>\n'
message += 'Build stderr <' + httpLinkToArchive + 'error.log>\n'

#Create Subject
subject = 'Subject: Mac Installer Build Report: '

if buildSuccess:
	subject += '[Build Successful]\n\n\n'
else:
	subject += '[Build Failed]\n\n\n'
	
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

sys.exit(1)
     
