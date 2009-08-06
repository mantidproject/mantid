import re
import smtplib
import os
from shutil import move
from time import strftime

#Email settings
smtpserver = 'outbox.rl.ac.uk'
localServerName = 'http://130.246.49.183/'
#localServerName = 'file://c|/Program Files/CruiseControl/'

RECIPIENTS = ['r.tolchenov@rl.ac.uk']
SENDER = 'Installer64@mantidproject.org'
if (os.name =='nt'):
     SENDER = 'Win' + SENDER
else:
     SENDER = 'Linux' + SENDER

#Set up email content 
buildSuccess = True

mssgBuild = ''
mssgBuildErr = ''

logDir = '../../../../logs/Installer/'

#create archive directory
archiveDir = logDir + strftime("%Y-%m-%d_%H-%M-%S")
os.mkdir(archiveDir)


#Get build result and errors
fileBuild = logDir+'build.log'
f = open(fileBuild,'r')

for line in f.readlines():
     buildResult = line
     mssgBuild = mssgBuild + line
     
f.close()
move(fileBuild,archiveDir)

if buildResult.startswith('failed'):
	buildSuccess = False	
	
fileBuildErr = logDir+'error.log'
mssgBuildErr = open(fileBuildErr,'r').read()
move(fileBuildErr,archiveDir)

if buildSuccess:
     exit(0)

#Construct Message
httpLinkToArchive = localServerName + archiveDir.replace('../../../../','') + '/'
message = 'Build Completed at: ' + strftime("%H:%M:%S %d-%m-%Y") + "\n"
message += 'Build Passed: ' + str(buildSuccess) + "\n"
message += 'BUILD LOG\n\n'
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

exit(1)
     
