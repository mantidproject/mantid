import sys
import stomp
import json
import time

def send(destination, message, persistent='true'):
    """
    Send a message to a queue
    @param destination: name of the queue
    @param message: message content
    """

    brokers = [("workflowdb1.sns.gov", 61613), ("workflowdb2.sns.gov", 61613)]
    icat_user="wfclient"
    icat_passcode="w0rkfl0w"

    conn = stomp.Connection(host_and_ports=brokers,
                    user=icat_user, passcode=icat_passcode,
                    wait_on_receipt=True)
    conn.start()
    conn.connect()
    conn.send(destination=destination, message=message, persistent=persistent)
    print "%s: %s" % ("destination", destination)
    print "%s: %s" % ("message", message)

    conn.disconnect()

destination = "POSTPROCESS.DATA_READY"
send(destination, sys.argv[1])
