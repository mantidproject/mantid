# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +


class ThreadModelWrapper(object):
    def __init__(self, function=None):
        self.callback = function

    def setInputs(self):
        pass

    def cancel(self):
        pass

    def execute(self):
        self.callback()


class ThreadModelWrapperWithOutput(object):
    def __init__(self, function, *args):
        self.callback = function
        self.args = args
        self.result = None

    def setInputs(self):
        pass

    def cancel(self):
        pass

    def execute(self):
        self.result = self.callback(*self.args)
