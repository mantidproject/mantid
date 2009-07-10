import urllib2
import socket
import os
import platform

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
    return versionList[0]
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
