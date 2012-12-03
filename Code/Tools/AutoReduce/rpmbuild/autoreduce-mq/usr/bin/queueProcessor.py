"""
Queue Processor
"""

import time, stomp, sys, logging, ConfigParser
from os import path, access, R_OK
from queueListener import QueueListener

class StreamToLogger(object):
    #Fake file-like stream object that redirects writes to a logger instance.
    def __init__(self, logger, log_level=logging.INFO):
        self.logger = logger
        self.log_level = log_level
        self.linebuf = ''
 
    def write(self, buf):
        for line in buf.rstrip().splitlines():
            self.logger.log(self.log_level, line.rstrip())

config = ConfigParser.RawConfigParser()
configFile="/etc/autoreduce/mq.properties"
if path.exists(configFile) and path.isfile(configFile) and access(configFile, R_OK):
    config.read('/etc/autoreduce/mq.properties')
elif path.isfile("mq.properties"):
    config.read('mq.properties')
else:
    print "Configure file /etc/autoreduce/mq.properties is missing"
    system.exit()

user = config.get('authentication', 'user')
password = config.get('authentication', 'password')
fname = config.get('log', 'filename')
try:
    with open(fname) as f: pass
except IOError as e:
    fname="/tmp/autoreduce.log"
    
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s:%(levelname)s:%(name)s:%(message)s',
    filename=fname,
    filemode='a'
)

stdout_logger = logging.getLogger('STDOUT')
sl = StreamToLogger(stdout_logger, logging.INFO)
sys.stdout = sl
 
stderr_logger = logging.getLogger('STDERR')
sl = StreamToLogger(stderr_logger, logging.ERROR)
sys.stderr = sl

brokers = [("workflowdb1.sns.gov", 61613), ("workflowdb2.sns.gov", 61613)]

queues = ['/queue/CATALOG.DATA_READY',
          '/queue/REDUCTION.DATA_READY',
          '/queue/REDUCTION_CATALOG.DATA_READY']
     
print "%s: %s" % ("brokers", brokers)

l = QueueListener(brokers, user, password, queues)
l.processing_loop()
