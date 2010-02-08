import re
import smtplib
import os
from shutil import move
from time import strftime
import sys
sys.path.insert(0,'../Mantid/Build')
import buildNotification as notifier

#Email settings
smtpserver = 'outbox.rl.ac.uk'

RECIPIENTS = ['mantid-buildserver@mantidproject.org']
SENDER = 'BuildServer64@mantidproject.org'
if (os.name =='nt'):
     SENDER = 'Win' + SENDER
else:
     SENDER = 'Linux' + SENDER

#Set up email content 
buildSuccess = True

mssgBuild = ''
mssgBuildErr = ''

project = 'qtiplot'
remoteArchiveDir, relativeLogDir = notifier.getArchiveDir(project)
localLogDir = '../../../../logs/' + project + '/'

#Get build result and errors
fileBuild = localLogDir+'build.log'
f = open(fileBuild,'r')

for line in f.readlines():
     buildResult = line
     mssgBuild = mssgBuild + line
     
f.close()
notifier.moveToArchive(fileBuild,remoteArchiveDir)

if buildResult.startswith('nmake failed'):
	buildSuccess = False	
	
fileBuildErr = localLogDir+'error.log'
mssgBuildErr = open(fileBuildErr,'r').read()
notifier.moveToArchive(fileBuildErr,remoteArchiveDir)

if buildSuccess:
     fileLaunchInstaller = localLogDir+'LaunchInstaller.txt'
     f = open(fileLaunchInstaller,'w')
     f.write('launch')
     exit(0)


#Construct Message
httpLinkToArchive = 'http://download.mantidproject.org/' + relativeLogDir.replace("\\","/")

message = 'Build Completed at: ' + strftime("%H:%M:%S %d-%m-%Y") + "\n"
message += 'QTIPlot Build Passed: ' + str(buildSuccess) + "\n"
message += 'QTIPLOT BUILD LOG\n\n'
message += 'Build stdout <' + httpLinkToArchive + 'build.log>\n'
message += 'Build stderr <' + httpLinkToArchive + 'error.log>\n'

#Create Subject
subject = 'Subject: '
if (os.name=='nt'):
     subject += "Windows"
else:
     subject += "Linux"
          
subject += ' Build Report: '

if buildSuccess:
	subject += '[QTIPlot Build Successful]\n\n\n'
else:
	subject += '[QTIPlot Build Failed]\n\n\n'
	
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

exit(1)
