# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy.QtCore import QRunnable, Slot, QThreadPool
import threading
import json
from .specifications import RundexSettings
import mantid.simpleapi as sapi
from mantid.kernel import config, logger

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

    def __init__(self):
        self.set_instrument(config['default.instrument'])
        self.data = list()

    def convolute(self, values):
        values = values[:-1]
        options = dict(zip(self.columns, values))
        custom_options = values[-1].split(',')
        for option in custom_options:
            key_value = option.split('=')
            if len(key_value) == 2:
                options.update({key_value[0]: key_value[1]})
        return options

    def process(self, elements):
        self.threadpool = QThreadPool()
        # TODO: check the elements before algorithm submission
        for e in elements:
            if (e < len(self.data) and len(self.data[e]) > 0):
                kwargs = self.convolute(self.data[e])
                kwargs.update(self.settings)
                self.execute(kwargs)

    def process_all(self):
        self.process(range(len(self.data)))

    def execute(self, kwargs):
        kwargs['OutputWorkspace'] = kwargs['SampleRuns']
        worker = DrillWorker(getattr(sapi, self.algorithm), **kwargs)
        worker.run()
        #self.threadpool.start(worker)

    def process_on_thread(self, elements):
        t = threading.Thread(target=self.process, args=(elements,))
        t.start()

    def set_instrument(self, instrument):
        self.data = list()
        if (instrument in RundexSettings.TECHNIQUE_MAP):
            config['default.instrument'] = instrument
            self.instrument = instrument
            self.technique = RundexSettings.get_technique(self.instrument)
            self.columns = RundexSettings.COLUMNS[self.technique]
            self.algorithm = RundexSettings.ALGORITHMS[self.technique]
        else:
            logger.error('Instrument {0} is not supported yet.'
                    .format(instrument))
            self.instrument = None
            self.technique = None
            self.columns = None
            self.algorithm = None

    def get_columns(self):
        return self.columns if self.columns is not None else list()

    def get_technique(self):
        return self.technique

    def add_row(self, position):
        self.data.insert(position, [""] * len(RundexSettings.COLUMNS[self.technique]))

    def del_row(self, position):
        del self.data[position]

    def change_data(self, row, column, contents):
        self.data[row][column] = contents

    def set_rundex_data(self, filename):
        with open(filename) as json_file:
            json_data = json.load(json_file)

        self.data = list()
        self.instrument = json_data["Instrument"]
        self.technique = RundexSettings.get_technique(self.instrument)
        self.columns = RundexSettings.COLUMNS[self.technique]
        for sample in range(len(json_data["Samples"])):
            self.data.append(list())
            for item in self.columns:
                if item in json_data["Samples"][sample]:
                    self.data[sample].append(
                            str(json_data["Samples"][sample][item]))
                else:
                    self.data[sample].append("")

    def get_rows_contents(self):
        return self.data

