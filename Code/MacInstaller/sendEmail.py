import re
import smtplib
import os
from shutil import move
from time import strftime
import sys
sys.path.append('../Mantid/Build')
import buildNotification as notifier
import urllib2
import time

base_url = "http://mantidlx1.isis.cclrc.ac.uk/~tzh47418/"
project = 'Installer'

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
remoteArchiveDir,relativeLogDir = notifier.getArchiveDir(project)

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

#get time taken to build
fileBuildTime = localLogDir + 'timebuild.log'
try:
    f = open(fileBuildTime, 'r')
    buildTime = float(f.read())
    f.close()
    buildNotification.moveToArchive(fileBuildTime, remoteArchivePath)
except IOError:
    buildTime = -1.0

# get SvnID
SvnID = notifier.getSVNRevision()

#build url for use in query
url = base_url + 'nfwd.psp?id=' + str(SvnID) + '&project=' + project
url += '&platform=' + platform.system() + platform.architecture()[0][:2]
url += '&result=' + str(buildSuccess) + '&time=' + str(buildTime)
# open url
try:
    f = urllib2.urlopen(url)
    time.sleep(0.1)
except urllib2.HTTPError:
    pass



if buildSuccess:
     sys.exit(0)

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
     
