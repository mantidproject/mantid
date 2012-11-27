import time, sys, stomp, json, os, imp
post_processing_bin = sys.path.append("/usr/bin") 
os.environ['NEXUSLIB'] = "/usr/lib64/libNeXus.so"

from ingestNexus_mq import IngestNexus
from ingestReduced_mq import IngestReduced

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
        destination = headers["destination"]
        data = json.loads(message)
        if data.has_key('data_file'):
            path = str(data['data_file'])
        else: 
            errorMsg = "data_file is missing"
            data["error"] = errorMsg
            print "Calling /queue/POSTPROCESS.ERROR with message %s" % message 
            self.send('/queue/POSTPROCESS.ERROR', json.dumps(data), persistent='true')
            return
        if destination == '/queue/REDUCTION.DATA_READY':
            param = path.split("/")
            if len(param) > 5:
                facility = param[1]
                instrument = param[2]
                proposal = param[3]
                out_dir = "/"+facility+"/"+instrument+"/"+proposal+"/shared/autoreduce/"
                reduce_script = "reduce_" + instrument
                reduce_script_path = "/" + facility + "/" + instrument + "/shared/autoreduce/" + reduce_script + ".py"
                print "reduce_script: %s" % reduce_script
                print "reduce_script_path: %s" % reduce_script_path
                print "out_dir: %s" % out_dir 
                print "Auto reducing: %s %s" % (path, out_dir)
                print "Calling /queue/REDUCTION.STARTED with message %s" % message 
                self.send('/queue/REDUCTION.STARTED', message, persistent='true')
                try:
                    m = imp.load_source(reduce_script, reduce_script_path)
                    reduction = m.AutoReduction(path, out_dir)
                    reduction.execute()
                    print "Calling /queue/REDUCTION.COMPLETE with message %s" % message 
                    self.send('/queue/REDUCTION.COMPLETE', message, persistent='true')
                except RuntimeError, e:
                    errorMsg = "REDUCTION RuntimeError: " + e.message
                    data["error"] = errorMsg
                    print "Calling /queue/POSTPROCESS.ERROR with message %s" % e.message 
                    self.send('/queue/POSTPROCESS.ERROR', json.dumps(data), persistent='true')
                except Exception, e:
                    errorMsg = "REDUCTION Error: " + e.message
                    data["error"] = errorMsg
                    print "Calling /queue/POSTPROCESS.ERROR with message %s" % errorMsg 
                    self.send('/queue/POSTPROCESS.ERROR', json.dumps(data), persistent='true')
            else:
                errorMsg = "REDUCTION Error: failed to parse data_file " + path
                data["error"] = errorMsg
                print "Calling /queue/POSTPROCESS.ERROR with message %s" % errorMsg 
                self.send('/queue/POSTPROCESS.ERROR', json.dumps(data), persistent='true')

        elif destination == '/queue/CATALOG.DATA_READY':
            self.send('/queue/CATALOG.STARTED', message, persistent='true')
            try:
                ingestNexus = IngestNexus(path)
                ingestNexus.execute()
                ingestNexus.logout()
                self.send('/queue/CATALOG.COMPLETE', message, persistent='true')
            except Exception, e:
                    errorMsg = "CATALOG Error: " + e.message
                    data["error"] = errorMsg
                    print "Calling /queue/POSTPROCESS.ERROR with message %s" % errorMsg 
                    self.send('/queue/POSTPROCESS.ERROR', json.dumps(data), persistent='true')

        elif destination == '/queue/REDUCTION_CATALOG.DATA_READY':
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
                        try:
                            print "Reduction Catalog: %s %s %s %s" % (facility, instrument, ipts, run_number)
                            self.send('/queue/REDUCTION_CATALOG.STARTED', message, persistent='true')
                            ingestReduced = IngestReduced(facility, instrument, ipts, run_number)
                            ingestReduced.execute()
                            ingestReduced.logout()
                            self.send('/queue/REDUCTION_CATALOG.COMPLETE', message, persistent='true')
                        except Exception, e:
                            errorMsg = "REDUCTION_CATALOG Catalog Error: " + e.message
                            data["error"] = errorMsg
                            print "Calling /queue/POSTPROCESS.ERROR with message %s" % errorMsg 
                            self.send('/queue/POSTPROCESS.ERROR', json.dumps(data), persistent='true')
            else:
                errorMsg = "REDUCTION_CATALOG Error: failed to parse data_file " + path
                data["error"] = errorMsg
                print "Calling /queue/POSTPROCESS.ERROR with message %s" % errorMsg 
                self.send('/queue/POSTPROCESS.ERROR', json.dumps(data), persistent='true')

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
