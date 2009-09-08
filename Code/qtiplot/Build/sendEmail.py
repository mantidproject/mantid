import re
import smtplib
import os
from shutil import move
from time import strftime

#Email settings
smtpserver = 'outbox.rl.ac.uk'
RECIPIENTS = ['mantid-buildserver@mantidproject.org']
SENDER = 'BuildServer1@mantidproject.org'
localServerName = 'http://' 
if (os.name =='nt'):
     SENDER = 'Win' + SENDER
     localServerName = localServerName + os.getenv('COMPUTERNAME') + '/'

else:
     SENDER = 'Linux' + SENDER
     localServerName = localServerName + os.getenv('HOSTNAME') + '/'

#Set up email content 
buildSuccess = True

mssgBuild = ''
mssgBuildErr = ''

logDir = '../../../../logs/qtiplot/'

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

if 'failed' in buildResult:
	buildSuccess = False	
	
fileBuildErr = logDir+'error.log'
mssgBuildErr = open(fileBuildErr,'r').read()
move(fileBuildErr,archiveDir)

if buildSuccess:
     fileLaunchInstaller = logDir+'LaunchInstaller.txt'
     f = open(fileLaunchInstaller,'w')
     f.write('launch')
     exit(0)


#Construct Message
httpLinkToArchive = localServerName + archiveDir.replace('../../../../','') + '/'
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
