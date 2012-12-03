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
            data["error"] = "data_file is missing"
            queue = '/queue/POSTPROCESS.ERROR'
            print "Calling %s with message %s " % (queue, json.dumps(data))
            self.send(queue, json.dumps(data), persistent='true')
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
                print "input file: %s: out directory: %s" % (path, out_dir)
                try:
                    print "Calling /queue/REDUCTION.STARTED with message %s" % message 
                    self.send('/queue/REDUCTION.STARTED', message, persistent='true')
                    m = imp.load_source(reduce_script, reduce_script_path)
                    reduction = m.AutoReduction(path, out_dir)
                    reduction.execute()
                    queue = '/queue/REDUCTION.COMPLETE'
                except RuntimeError, e:
                    data["error"] = "REDUCTION RuntimeError: " + ''.join(e) 
                    queue = '/queue/POSTPROCESS.ERROR'
                except KeyError, e:
                    data["error"] = "REDUCTION KeyError: " + ''.join(e)
                    queue = '/queue/POSTPROCESS.ERROR'
                except Exception, e:
                    data["error"] = "REDUCTION Error: " + ''.join(e)
                    queue = '/queue/POSTPROCESS.ERROR'
                finally:
                    print "Calling %s with message %s " % (queue, json.dumps(data))
                    self.send(queue, json.dumps(data), persistent='true')
            else:
                data["error"] = "REDUCTION Error: failed to parse data_file " + path
                print "Calling /queue/POSTPROCESS.ERROR with message %s" % json.dumps(data) 
                self.send('/queue/POSTPROCESS.ERROR', json.dumps(data), persistent='true')

        elif destination == '/queue/CATALOG.DATA_READY':
            try:
                self.send('/queue/CATALOG.STARTED', message, persistent='true')
                ingestNexus = IngestNexus(path)
                ingestNexus.execute()
                ingestNexus.logout()
                queue = '/queue/CATALOG.COMPLETE'
            except Exception, e:
                    data["error"] = "CATALOG Error: " + ''.join(e)
                    queue = '/queue/POSTPROCESS.ERROR'
            finally:
                    print "Calling %s with message %s " % (queue, json.dumps(data))
                    self.send(queue, json.dumps(data), persistent='true')

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
                            queue = '/queue/REDUCTION_CATALOG.COMPLETE'
                        except Exception, e:
                            data["error"] = "REDUCTION_CATALOG Catalog Error: " + ''.join(e)
                            queue = '/queue/POSTPROCESS.ERROR'
                        finally:
                            print "Calling %s with message %s " % (queue, json.dumps(data))
                            self.send(queue, json.dumps(data), persistent='true')
            else:
                data["error"] = "REDUCTION_CATALOG Error: failed to parse data_file " + path
                queue = '/queue/POSTPROCESS.ERROR'
                print "Calling %s with message %s " % (queue, json.dumps(data))
                self.send(queue, json.dumps(data), persistent='true')

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
