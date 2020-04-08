# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy.QtCore import QObject, QThread, Signal
import threading
import json
from .specifications import RundexSettings
import mantid.simpleapi as sapi
from mantid.kernel import config, logger

class DrillWorker(QThread):

    process_done = Signal()

    def __init__(self):
        super(DrillWorker, self).__init__()
        self.processes = list()
        self.stopped = False

    def add_process(self, algo, **kwargs):
        self.processes.append((algo, kwargs))

    def stop(self):
        self.stopped = True
        for process in self.processes:
            p = sapi.AlgorithmManager.newestInstanceOf(process[0])
            if p:
                p.cancel()

    def run(self):
        for process in self.processes:
            if self.stopped:
                return
            try:
                fn = getattr(sapi, process[0])
                fn(**process[1])
                self.process_done.emit()
            except Exception as e:
                print(e)
            except:
                pass


class DrillModel(QObject):

    settings = dict()
    algorithm = None
    process_done = Signal()
    processing_done = Signal()

    def __init__(self):
        super(DrillModel, self).__init__()
        self.set_instrument(config['default.instrument'])
        self.samples = list()
        self.thread = None
        self.processes_running = 0
        self.processes_done = 0

    def convolute(self, options):
        local_options = dict()
        local_options.update(options)
        if "CustomOptions" in local_options:
            local_options.update(local_options["CustomOptions"])
            del local_options["CustomOptions"]
        return local_options

    def process(self, elements):
        self.thread = DrillWorker()
        self.processes_running = 0
        self.processes_done = 0
        # TODO: check the elements before algorithm submission
        for e in elements:
            if (e < len(self.samples) and len(self.samples[e]) > 0):
                kwargs = self.convolute(self.samples[e])
                kwargs.update(self.settings)
                kwargs['OutputWorkspace'] = "sample_" + str(e)
                self.thread.add_process(self.algorithm, **kwargs)
                self.processes_running += 1
        self.thread.process_done.connect(self.on_process_done)
        self.thread.finished.connect(self.on_processing_done)
        self.thread.start()

    def stop_process(self):
        self.thread.stop()
        self.processing_done.emit()

    def on_process_done(self):
        self.processes_done += 1
        self.process_done.emit()

    def on_processing_done(self):
        self.processing_done.emit()

    def set_instrument(self, instrument):
        self.samples = list()
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
        self.samples.insert(position, dict())

    def del_row(self, position):
        del self.samples[position]

    def change_data(self, row, column, contents):
        if (not contents):
            if (self.columns[column] in self.samples[row]):
                del self.samples[row][self.columns[column]]
        elif (self.columns[column] == self.columns[-1]):
            options = dict()
            for option in contents.split(","):
                options[option.split("=")[0]] = option.split("=")[1]
            self.samples[row][self.columns[column]] = options
        else:
            self.samples[row][self.columns[column]] = contents

    def set_rundex_data(self, filename):
        with open(filename) as json_file:
            json_data = json.load(json_file)

        self.samples = list()
        self.instrument = json_data["Instrument"]
        self.technique = RundexSettings.get_technique(self.instrument)
        self.columns = RundexSettings.COLUMNS[self.technique]

        for sample in json_data["Samples"]:
            self.samples.append(sample)

    def export_rundex_data(self, filename):
        json_data = dict()
        # header TODO: to be continued
        json_data["Instrument"] = self.instrument
        json_data["Technique"] = self.technique

        # TODO: add settings here

        # samples
        json_data["Samples"] = list()
        for sample in self.samples:
            json_data["Samples"].append(sample)

        with open(filename, 'w') as json_file:
            json.dump(json_data, json_file, indent=4)

    def get_rows_contents(self):
        rows = list()
        for sample in self.samples:
            row = list()
            for column in self.columns[:-1]:
                if column in sample:
                    row.append(str(sample[column]))
                else:
                    row.append("")
            if self.columns[-1] in sample:
                options = str()
                for (k, v) in sample[self.columns[-1]].items():
                    options += str(k) + "=" + str(v)
                row.append(options)
            rows.append(row)
        return rows

    def get_supported_techniques(self):
        return [technique for technique in RundexSettings.ALGORITHMS]

    def get_processing_progress(self):
        return self.processes_done, self.processes_running

