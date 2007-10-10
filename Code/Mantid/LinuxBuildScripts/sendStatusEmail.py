import smtplib

smtpserver = 'outbox.rl.ac.uk'

RECIPIENTS = ['m.j.clarke@rl.ac.uk']#, 'n.draper@rl.ac.uk', 'russell.taylor@rl.ac.uk','l.c.chapon@rl.ac.uk', 'f.a.akeroyd@rl.ac.uk', 'd.champion@rl.ac.uk', 's.ansell@rl.ac.uk', 'a.j.markvardsen@rl.ac.uk']
#,'mantid-developers@mantidproject.org'
SENDER = 'BuildServer1@mantidproject.org'

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