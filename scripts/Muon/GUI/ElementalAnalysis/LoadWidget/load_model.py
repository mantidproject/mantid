import os

import mantid.simpleapi as mantid


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

        file = self._format_path(self.filename)
        self.alg.setProperty("OutputWorkspace", file)
        self.alg.execute()
        mantid.AnalysisDataService.addOrReplace(
            file, self.alg.getProperty("OutputWorkspace").value)

    def loadData(self, inputs):
        self.filename = inputs["Filename"]

    def _format_path(self, path):
        file = "WS_{}".format(os.path.basename(self.filename))
        return file[:file.rfind(".")]
