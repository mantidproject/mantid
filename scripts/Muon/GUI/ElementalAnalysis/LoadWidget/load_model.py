import mantid.simpleapi as mantid

from six import iteritems

class LoadModel(object):
    def __init__(self):
        self.alg = None
        self.filename = ""

    def cancel(self):
        if self.alg is not None:
            self.alg.cancel()

    def output(self):
        return

    def execute(self):
        if self.filename == "":
            self.cancel()
            return
        self.alg = mantid.AlgorithmManager.create("Load")   
        self.alg.initialize()
        self.alg.setAlwaysStoreInADS(False)
        self.alg.setProperty("Filename", self.filename)

        #l = lambda x: "WS_{}".format(x)
        self.alg.setProperty("OutputWorkspace", "WS_{}".format(self.filename))
        self.alg.execute()
        mantid.AnalysisDataService.addOrReplace("WS_{}".format(self.filename), self.alg.getProperty("OutputWorkspace").value)

    def loadData(self, inputs):
        self.filename = inputs["Filename"]
        

    
