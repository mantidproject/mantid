import smtplib
from time import strftime

#Email settings
smtpserver = 'outbox.rl.ac.uk'

RECIPIENTS = ['m.j.clarke@rl.ac.uk', 'n.draper@rl.ac.uk', 'russell.taylor@rl.ac.uk','l.c.chapon@rl.ac.uk', 'f.a.akeroyd@rl.ac.uk', 'd.champion@rl.ac.uk', 's.ansell@rl.ac.uk', 'a.j.markvardsen@rl.ac.uk']
#,'mantid-developers@mantidproject.org'
SENDER = 'BuildServer1@mantidproject.org'

#Set up email content
buildSuccess = False
sconsResult = ''
testsPass = True
mssgScons = ''
mssgSconsErr = ''
mssgTests = ''
mssgSvn  = ''
mssgDoxy = ''

#Get scons result and errors
f = open('../logs/scons.log','r')

for line in f.readlines():
     sconsResult = line
     mssgScons = mssgScons + line
     
f.close()

if sconsResult.startswith('scons: done building targets.'):
	buildSuccess = True	
	
mssgSconsErr = open('../logs/sconsErr.log','r').read()

#Get tests result
f = open('../logs/testResults.log','r')

for line in f.readlines():
	print line
	if line.startswith('Failed ')  != -1 and line.endswith(' test\n'):
		#A test failed
		testsPass = False
	mssgTests = mssgTests + line
     
f.close()


#Read svn log
mssgSvn = open('../logs/svn.log','r').read()

#Read doxygen log
mssgDoxy = open('../logs/doxy.log','r').read()

#Construct Message

message = 'Build Completed at: ' + strftime("%H:%M:%S %d-%m-%Y") + "\n"
message += 'Build Passed: ' + str(buildSuccess) + "\n"
message += 'Units Tests Passed: ' + str(testsPass) + "\n\n"
message += mssgSvn + "\n"
message += 'BUILD LOG\n\n'
message += mssgScons + "\n\n"
message += mssgSconsErr + "\n"
message += '------------------------------------------------------------------------\n'
message += 'UNIT TEST LOG\n\n'
message += mssgTests + "\n"
message += '------------------------------------------------------------------------\n'
message += 'DOXYGEN LOG\n\n'
message += mssgDoxy + "\n"

#Create Subject
subject = 'Subject: Build Report: '

if buildSuccess:
	subject += '[Build Successful, '
else:
	subject += '[Build Failed, '
	
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
