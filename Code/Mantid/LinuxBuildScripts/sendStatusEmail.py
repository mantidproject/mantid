import smtplib

smtpserver = 'outbox.rl.ac.uk'

RECIPIENTS = ['m.j.clarke@rl.ac.uk']
SENDER = 'BuildServer1-DO_NOT_REPLY@rl.ac.uk'

mssg1 = open('../logs/scons.log', 'r').read()

mssg2 = open('../logs/testResults.log', 'r').read()

session = smtplib.SMTP(smtpserver)

smtpresult = session.sendmail(SENDER, RECIPIENTS, 'Subject: Build Report\n' + mssg1 + '\n' + mssg2)

if smtpresult:
    errstr = ""
    for recip in smtpresult.keys():
        errstr = """Could not delivery mail to: %s

Server said: %s
%s

%s""" % (recip, smtpresult[recip][0], smtpresult[recip][1], errstr)
    raise smtplib.SMTPException, errstr