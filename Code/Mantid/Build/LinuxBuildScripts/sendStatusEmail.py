import re
import smtplib
from time import strftime

#Email settings
smtpserver = 'outbox.rl.ac.uk'

RECIPIENTS = ['mantid-buildserver@mantidproject.org'] #['m.j.clarke@rl.ac.uk']
#,'mantid-developers@mantidproject.org'
SENDER = 'BuildServer1@mantidproject.org'

#Set up email content
buildSuccess = False
testsBuildSuccess = False
sconsResult = ''
testsResult = ''
testsPass = True
pythonPass = True

mssgScons = ''
mssgSconsErr = ''
mssgTestsBuild = ''
mssgTestsErr = ''
mssgTestsRunErr = ''
mssgTestsResults = ''
mssgPythonResults = ''
mssgSvn  = ''
mssgDoxy = ''

#Get scons result and errors
f = open('../../../../logs/scons.log','r')

for line in f.readlines():
     sconsResult = line
     mssgScons = mssgScons + line
     
f.close()

if sconsResult.startswith('scons: done building targets.'):
	buildSuccess = True	

# Count compilation warnings
reWarnCount = re.compile(": warning")
wc = reWarnCount.findall(mssgScons)
compilerWarnCount = len(wc)

mssgSconsErr = open('../../../../logs/sconsErr.log','r').read()

#Get tests scons result and errors
f = open('../../../../logs/testsBuild.log','r')

for line in f.readlines():
     testsResult = line
     mssgTestsBuild = mssgTestsBuild + line
     
f.close()

if testsResult.startswith('scons: done building targets.'):
	testsBuildSuccess = True	
	
mssgTestsErr = open('../../../../logs/testsBuildErr.log','r').read()

f = open('../../../../logs/testsRunErr.log','r')

for line in f.readlines():
	temp = line
	if temp.startswith('TestsScript.sh:'):
		testsBuildSuccess = False
		mssgTestsRunErr  = mssgTestsRunErr + temp[0:temp.find('>>')] + '\n'
     
f.close()

#Get tests result
f = open('../../../../logs/testResults.log','r')

for line in f.readlines():
	if line.startswith('Failed ')  != -1 and line.endswith(' tests\n'):
		#A test failed
		testsPass = False
	if line.startswith('Failed ')  != -1 and line.endswith(' test\n'):
		#A test failed
		testsPass = False
	mssgTestsResults = mssgTestsResults + line
     
f.close()

#Get python tests result
#~ try:
	#~ f = open('../../../../logs/PythonResults.log','r')

	#~ for line in f.readlines():
		#~ if line.endswith('FAILED'):
			#~ #A test failed
			#~ pythonPass = False
		#~ mssgPythonResults = mssgPythonResults + line
     
	#~ f.close()

#~ except:
	#~ pythonPass = False

#Read svn log
mssgSvn = open('../../../../logs/svn.log','r').read()

#Read doxygen log
mssgDoxy = open('../../../../logs/doxy.log','r').read()

#Construct Message

message = 'Build Completed at: ' + strftime("%H:%M:%S %d-%m-%Y") + "\n"
message += 'Framework Build Passed: ' + str(buildSuccess)
if compilerWarnCount>0:
  message += " (" + str(compilerWarnCount) + " compiler warnings)"
message += '\n'
message += 'Tests Build Passed: ' + str(testsBuildSuccess) + "\n"
message += 'Units Tests Passed: ' + str(testsPass) + "\n\n"
#~ message += 'Python Tests Passed: ' + str(pythonPass) + "\n\n"
message += mssgSvn + "\n"
message += 'UNIT TESTS LOG\n\n'
message += mssgTestsResults + "\n"

#~ message += '------------------------------------------------------------------------\n'
#~ message += 'PYTHON TESTS LOG\n\n'
#~ message += mssgPythonResults + "\n"
message += '------------------------------------------------------------------------\n'
message += 'FRAMEWORK BUILD LOG\n\n'
message += mssgScons + "\n\n"
message += mssgSconsErr + "\n"
message += '------------------------------------------------------------------------\n'
message += 'TESTS BUILD LOG\n\n'
message += mssgTestsBuild + "\n\n"
message += mssgTestsErr + "\n"
message += mssgTestsRunErr  + "\n"
message += '------------------------------------------------------------------------\n'
message += 'DOXYGEN LOG\n\n'
message += mssgDoxy + "\n"

print mssgTestsResults

#Create Subject
subject = 'Subject: Build Report: '

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
