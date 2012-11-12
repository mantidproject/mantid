import sys, os, stomp, json, time, ConfigParser

def send(destination, message, persistent='true'):
    """
    Send a message to a queue
    @param destination: name of the queue
    @param message: message content
    """
    config = ConfigParser.RawConfigParser()
    configFile="/etc/autoreduce/mq.properties"
    if os.path.exists(configFile) and os.path.isfile(configFile):
        config.read('/etc/autoreduce/mq.properties')
    elif os.path.isfile("mq.properties"):
        config.read('mq.properties')
    else:
        print "Configure file /etc/autoreduce/mq.properties is missing"
        system.exit()
    
    user = config.get('authentication', 'user')
    password = config.get('authentication', 'password')

    print user, password
    brokers = [("workflowdb1.sns.gov", 61613), ("workflowdb2.sns.gov", 61613)]

    conn = stomp.Connection(host_and_ports=brokers,
                    user=user, passcode=password,
                    wait_on_receipt=True)
    conn.start()
    conn.connect()
    conn.send(destination=destination, message=message, persistent=persistent)
    print "%s: %s" % ("destination", destination)
    print "%s: %s" % ("message", message)

    conn.disconnect()

def main():
    print sys.argv[1]
    destination = "POSTPROCESS.DATA_READY"
    send(destination, sys.argv[1])

if __name__ == '__main__':
  main()
