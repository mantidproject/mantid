import smtplib
import argparse
from email.mime.text import MIMEText

class SendEmailSecure:
    def __init__(self, sender, password, recipient, regLinks, speedLinks):
        self.sender = sender
        self.password = password
        self.recipient = recipient
        self.regLinks = regLinks
        self.speedLinks = speedLinks
    
    def send(self):
        msg = MIMEText("The following tests may have performance regressions:\n"+"\n\n - ".join(self.regLinks)+"\n This following tests may have spedup:\n"+"\n\n - ".join(self.speedLinks))

        msg['Subject'] = 'Possible Performance Regressions'
        msg['From'] = self.sender
        msg['To'] = self.recipient

        # Send the message via our own SMTP server, but don't include the
        # envelope header.
        s = smtplib.SMTP("smtp.gmail.com", 587)
        s.ehlo()
        s.starttls()
        s.login(self.sender, self.password)
        s.sendmail(self.sender, [self.recipient], msg.as_string())
        s.quit()

if __name__ == "__main__":
    # Parse the command line
    parser = argparse.ArgumentParser(description='Sends a secure email with test links from the mantid gmail account to some recipient developer')

    parser.add_argument('sender', type=str, default="mantidproject@gmail.com",
                        help='Gmail email address')

    parser.add_argument('pwd', type=str, help='password for gmail address')
    parser.add_argument('recipient', type=str, help='recipient email address')
    parser.add_argument('regLinks', type=str, help='links to tests which have regressed')
    parser.add_argument('speedLinks', type=str, help='links to tests which have sped up')
    args = parser.parse_args()
    email = SendEmailSecure(args.sender, args.pwd, args.recipient, args.regLinks.split(";"), args.speedLinks.split(";"))
    email.send()
    
    