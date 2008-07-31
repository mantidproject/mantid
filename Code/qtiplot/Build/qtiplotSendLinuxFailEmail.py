import smtplib
from time import strftime

#Email settings
smtpserver = 'outbox.rl.ac.uk'

RECIPIENTS = ['m.j.clarke@rl.ac.uk']#'mantid-buildserver@mantidproject.org']
#,'mantid-developers@mantidproject.org'
SENDER = 'BuildServer1@mantidproject.org'

#Set up email content
buildSuccess = True

errLog = ''

#Get tests result
f = open('../../logs/qtiplotErr.log','r')

#if err log ends with something like:
#make: *** [../tmp/qtiplot/ApplicationWindow.o] Error 1
#then it has failed
for line in f.readlines():
	if line.startswith('make: ')  != -1 and line.count(' Error ') > 0:
		buildSuccess = False
	errLog = errLog + line
f.close()


#Construct Message

if buildSuccess == False:
	message = 'Build Attempted at: ' + strftime("%H:%M:%S %d-%m-%Y") + "\n"
	message += 'QTIPLOT LINUX BUILD: FAILED\n\n'

	message += '------------------------------------------------------------------------\n'
	message += 'ERROR LOG\n\n'
	message += errLog + "\n"
	message += '------------------------------------------------------------------------\n'

	#Create Subject
	subject = 'Subject: QtiPlot Linux Build Failed\n'

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
