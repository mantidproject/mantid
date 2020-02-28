# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy.QtCore import QRunnable, Slot, QThreadPool
import threading
from .specifications import RundexSettings
import mantid.simpleapi as sapi


class DrillWorker(QRunnable):

    def __init__(self, fn, *args, **kwargs):
        super(DrillWorker, self).__init__()
        self.fn = fn
        self.args = args
        self.kwargs = kwargs

    @Slot()
    def run(self):
        self.fn(*self.args, **self.kwargs)


class DrillModel(object):

    settings = dict()
    threadpool = None
    algorithm = None

    def __init__(self, instrument):
        self.instrument = instrument
        self.technique = RundexSettings.get_technique(instrument)
        self.columns = RundexSettings.COLUMNS[self.technique]
        self.algorithm = RundexSettings.ALGORITHMS[self.technique]

    def convolute(self, values):
        values = values[:-1]
        options = dict(zip(self.columns, values))
        custom_options = values[-1].split(',')
        for option in custom_options:
            key_value = option.split('=')
            if len(key_value) == 2:
                options.update({key_value[0]: key_value[1]})
        return options

    def process(self, contents):
        self.threadpool = QThreadPool()
        for row in contents:
            kwargs = self.convolute(row)
            kwargs.update(self.settings)
            self.execute(kwargs)

    def execute(self, kwargs):
        kwargs['OutputWorkspace'] = kwargs['SampleRuns']
        worker = DrillWorker(getattr(sapi, self.algorithm), **kwargs)
        worker.run()
        #self.threadpool.start(worker)

    def process_on_thread(self, contents):
        t = threading.Thread(target=self.process, args=(contents,))
        t.start()

