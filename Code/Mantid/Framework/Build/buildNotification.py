import urllib2
import smtplib
import socket
import os
import sys
import platform
from time import strftime
import subprocess as sp
import shutil

base_url = "http://download.mantidproject.org/build_metrics/"

def buildURL(messageDictionary):
  url= "http://ndlt343/build.psp?"
  for arg in messageDictionary:
    value = messageDictionary[arg]
    url += str(arg) + "=" + str(value) + "&"
  #strip trailing & and ?
  url = url.rstrip('&?')
  url = url.replace(" ","%20")
  return url
#end def

def sendNotification(messageDictionary,useProxy=0):
  # timeout in seconds
  timeout = 5
  socket.setdefaulttimeout(timeout)
  
  if useProxy:
    #set proxy
    proxies = {'http': 'http://wwwcache.rl.ac.uk:8080/'}
  else:
    proxies = {}
  proxy_support = urllib2.ProxyHandler(proxies)
  opener = urllib2.build_opener(proxy_support)
  urllib2.install_opener(opener)
  
  message=buildURL(messageDictionary)
  #print message
  try:
    f = urllib2.urlopen(message) 
    return 1
  except:
    if useProxy==0:
      return sendNotification(messageDictionary,1)
    return 0
#end def 

def getSVNRevision():
  put, get = os.popen4("svnversion .")
  line=get.readline()
  #remove non alphanumeric indicators
  line=line.replace("M","")
  line=line.replace("S","")
  line=line.replace("P","")
  line=line.strip("")
  versionList = line.split(":")
  #get the max version no
  try:
    maxVersion = 0
    for versionStr in versionList:
      if int(versionStr) > maxVersion:
        maxVersion = int(versionStr)
    return str(maxVersion)
  except:
    return versionList[0].rstrip()
#end def

def getArchitecture():
  architecture = platform.system()
  if architecture == "Windows":
    architecture += platform.release()
  (bits,linkage)=platform.architecture()
  architecture += bits
  return architecture.rstrip('bits')
#end def

def createKeyDictionary(project):
  messageDictionary = {}
  messageDictionary['svn']=getSVNRevision()
  messageDictionary['project']=project
  messageDictionary['arch']=getArchitecture()
  return messageDictionary
#end def

def sendBuildStarted(project):
  messageDictionary = createKeyDictionary(project)
  messageDictionary['status']="Build Started"
  return sendNotification(messageDictionary)
#end def

def sendBuildCompleted(project):
  messageDictionary = createKeyDictionary(project)
  messageDictionary['status']="Build Completed"
  return sendNotification(messageDictionary)
#end def

def sendTestBuildStarted(project):
  messageDictionary = createKeyDictionary(project)
  messageDictionary['status']="TestBuild Started"
  return sendNotification(messageDictionary)
#end def

def sendTestBuildCompleted(project):
  messageDictionary = createKeyDictionary(project)
  messageDictionary['status']="TestBuild Completed"
  return sendNotification(messageDictionary)
#end def

def sendTestStarted(project):
  messageDictionary = createKeyDictionary(project)
  messageDictionary['status']="Test Started"
  return sendNotification(messageDictionary)
#end def

def sendTestCompleted(project,testCount=0,testFail=0, \
                        compWarn=0,docuWarn=0,buildErrors=0, \
                        testBuildErrors=0):
  messageDictionary = createKeyDictionary(project)
  messageDictionary['status']="Test Completed"
  messageDictionary['tc']=str(testCount)
  messageDictionary['tf']=str(testFail)  
  messageDictionary['cw']=str(compWarn)
  messageDictionary['dw']=str(docuWarn)
  messageDictionary['be']=str(buildErrors)
  messageDictionary['tbe']=str(testBuildErrors)
  return sendNotification(messageDictionary)
#end def

# Return the relative path the logs
def getLogDir(project):
  archiveDir = strftime("%Y-%m-%d_%H-%M-%S")
  if os.name == 'nt':
    # MG 29/01/10: Temporarily moved back to local machine until permissions sorted
    system = platform.system()
    if platform.architecture()[0] == '64bit':
        system += '64'
    logDir = "logs\\" + system + "\\" + project + "\\" + archiveDir + "\\"
  else:
    logDir = 'logs/' + platform.system() + '/' + project + '/' + archiveDir + '/'
  return logDir
    
# Create a path that points to the archive directory
def getArchiveDir(project):
  machine = 'Shadow.nd.rl.ac.uk'
  logDir = getLogDir(project)
  if os.name == 'nt':
    baseDir = "\\\\" + machine + "\\mantidkits$\\"
    try:
      os.makedirs(baseDir + logDir)
    except WindowsError:
      pass
  else:
    baseDir = '/isis/www/mantidproject_download/'
    sp.call("ssh mantidlog@" + machine + " \"mkdir -p " + baseDir + logDir + "\"",shell=True)
    
  #create archive directory
  archivePath = baseDir + logDir
  if os.name == 'posix':
    archivePath = "mantidlog@" + machine + ':' + archivePath
  return archivePath, logDir

# Move a file to a directory
def moveToArchive(logfile, archiveDir):
  if os.name == 'nt':
    # Standard move works for network drives as well
    shutil.move(logfile,archiveDir)
  else:
    # Need to scp logs to destination
    if not archiveDir.endswith('/'):
      archiveDir += '/'
    locallogs = os.path.dirname(logfile)
    filename = os.path.basename(logfile)
    scplog = open(locallogs + '/scp.log','w')
    sp.call("scp " + logfile + " " + archiveDir + filename,stdout=scplog,shell=True)
    scplog.close()

# Move send email
def sendResultMail(message, logdir, sender, recipients=['mantid-buildserver@mantidproject.org'], \
                   mailserver="outbox.nd.rl.ac.uk"):
  #timeout in seconds
  socket.setdefaulttimeout(60)
  logfile = logdir + 'email.log'
  emailErr = open(logfile,'w')
  try:
     #Send Email
     session = smtplib.SMTP(mailserver)
  except (smtplib.SMTPException, smtplib.socket.error), details:
    # Catch time out errors
    session = None
    emailErr.write(str(details) + "\n")

  if session != None:
    try:
      smtpresult = session.sendmail(sender, recipients, message)
    except (smtplib.SMTPException, smtplib.socket.error), details:
      emailErr.write(str(details) + "\n")
      smtpresult = None

    if smtpresult != None:
      errstr = ""
      for recip in smtpresult.keys():
           errstr = """Could not deliver mail to: %s
           Server said: %s
           %s
           %s""" % (recip, smtpresult[recip][0], smtpresult[recip][1], errstr)
           emailErr.write(errstr)
    session.close()
  return logfile
