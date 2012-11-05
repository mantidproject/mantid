import time, sys, stomp, json
post_processing_bin = sys.path.append("/usr/bin") 
from ingestNexus_mq import IngestNexus
from ingestReduced_mq import IngestReduced
from reduce_HYSA import AutoReduction
import logging
logging.getLogger().setLevel(logging.INFO)

class QueueListener(stomp.ConnectionListener):

    def __init__(self, brokers, user, passcode, queues=[]):
        self._delay = 5.0
        self._connection = None
        self._connected = False
        self._brokers = brokers
        self._user = user
        self._passcode = passcode
        self._queues = queues

    def on_receipt(self, headers, body):
        print "RECEIPT: %s" % headers

    def send(self, destination, message, persistent='true'):
        self._connection.send(destination=destination, message=message, persistent=persistent)
        print " -> %s" % destination

    def on_message(self, headers, message):
        print "<--- %s: %s" % (headers["destination"], message)
        gotit = False
        destination = headers["destination"]
        if destination=='/queue/REDUCTION.DATA_READY':
            data = json.loads(message)
            if data.has_key('data_file'):
                path = str(data['data_file'])
                param = path.split("/")
                if len(param) > 5:
                    facility = param[1]
                    instrument = param[2]
                    proposal = param[3]
                    out_dir = "/"+facility+"/"+instrument+"/"+proposal+"/shared/autoreduce"
                    reduce_script="/"+facility+"/"+instrument+"/shared/autoreduce"+"reduce_" + instrument + ".py"
                    print "reduce_script: %s" % reduce_script
                    print "Auto reducing: %s %s" % (path, out_dir)
                    self.send('/queue/REDUCTION.STARTED', message, persistent='true')
                    reduction = AutoReduction(path, out_dir)
                    reduction.execute()
                    self.send('/queue/REDUCTION.COMPLETE', message, persistent='true')
                else:
                    errorMsg = "faied to parse data_file " + path
                    self.send('/queue/REDUCTION.ERROR', message, persistent='true')
            else: 
                errorMsg = "data_file is missing"
                self.send('/queue/REDUCTION.ERROR', errorMsg, persistent='true')
            gotit = True
        elif destination=='/queue/CATALOG.DATA_READY':
            data = json.loads(message)
            if data.has_key('data_file'):
                path = str(data['data_file'])
                self.send('/queue/CATALOG.STARTED', message, persistent='true')
                ingestNexus = IngestNexus(path)
                ingestNexus.execute()
                ingestNexus.logout()
                self.send('/queue/CATALOG.COMPLETE', message, persistent='true')
            else:
                errorMsg = "data_file is missing"
                self.send('/queue/CATALOG.ERROR', message, persistent='true')
            gotit = True
        elif destination=='/queue/REDUCTION_CATALOG.DATA_READY':
            data = json.loads(message)
            if data.has_key('data_file'):
                path = str(data['data_file'])
                param = path.split("/")
                if len(param) > 5:
                    facility = param[1]
                    instrument = param[2]
                    ipts = param[3]
                    filename = param[5]
                
                    param2 = filename.split(".")
                    if len(param2) > 2:
                        param3 = param2[0].split("_")
                        if len(param3) > 1:
                            run_number = param3[1]
                            print "Reduction Catalog: %s %s %s %s" % (facility, instrument, ipts, run_number)
                            self.send('/queue/REDUCTION_CATALOG.STARTED', message, persistent='true')
                            ingestReduced = IngestReduced(facility, instrument, ipts, run_number)
                            ingestReduced.execute()
                            ingestReduced.logout()
                            self.send('/queue/REDUCTION_CATALOG.COMPLETE', message, persistent='true')
                else:
                    errorMsg = "faied to parse data_file " + path
                    self.send('/queue/REDUCTION_CATALOG.ERROR', message, persistent='true')

            else:
                errorMsg = "data_file is missing"
                self.send('/queue/REDUCTION_CATALOG.ERROR', message, persistent='true')

            gotit = True

    def connect(self):
        # Do a clean disconnect first
        self._disconnect()
        conn = stomp.Connection(host_and_ports=self._brokers,
                                user=self._user,
                                passcode=self._passcode,
                                wait_on_receipt=True)
        conn.set_listener('worker_bee', self)
        conn.start()
        conn.connect()
        for q in self._queues:
            conn.subscribe(destination=q, ack='auto', persistent='true')
        self._connection = conn
        self._connected = True
        logging.info("Connected to %s:%d\n" % conn.get_host_and_port())

    def on_disconnected(self):
        self._connected = False

    def _disconnect(self):
        if self._connection is not None and self._connection.is_connected():
            self._connection.disconnect()
        self._connection = None

    def listen_and_wait(self, waiting_period=1.0):
        self.connect()
        while(self._connected):
            time.sleep(waiting_period)

    def processing_loop(self):
        print "processing"
        _listen = True
        conn = None
        while(_listen):
            try:
                # Get the next message in the post-processing queue
                self.listen_and_wait(self._delay)
            except KeyboardInterrupt:
                print "\nStopping"
                _listen = False
            finally:
                self._disconnect()
