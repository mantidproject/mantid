# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import json

from mantid.kernel import *

from .configurations import RundexSettings
from .DrillSample import DrillSample


class DrillRundexIO:

    """
    Name of the file where the data will be read or saved.
    """
    _filename = str()

    """
    Drill model. Used to set and get the data.
    """
    _drillModel = None

    def __init__(self, filename, drillModel):
        """
        Init the IO model by setting the filename and the data model.

        Args:
            filename (str): absolute path of the file where the data will be
                            read and saved
            drillModel (DrillModel): data model
        """
        super(DrillRundexIO, self).__init__()
        self._filename = filename
        self._drillModel = drillModel

    def getFilename(self):
        """
        Get the file name and full path.

        Return:
            str: absolute path to the file
        """
        return self._filename

    def load(self):
        """
        Import data and set the associated model.
        """
        drill = self._drillModel
        with open(self._filename) as json_file:
            try:
                json_data = json.load(json_file)
            except Exception as ex:
                logger.error("Wrong file format for: {0}"
                             .format(self._filename))
                logger.error(str(ex))
                raise ex

        if ((RundexSettings.MODE_JSON_KEY not in json_data)
                or (RundexSettings.INSTRUMENT_JSON_KEY not in json_data)):
            logger.error("Unable to load {0}".format(self._filename))
            raise ValueError("Json mandatory fields '{0}' or '{1}' not found."
                             .format(RundexSettings.CYCLE_JSON_KEY,
                                     RundexSettings.INSTRUMENT_JSON_KEY))

        drill.setInstrument(json_data[RundexSettings.INSTRUMENT_JSON_KEY])
        drill.setAcquisitionMode(json_data[RundexSettings.MODE_JSON_KEY])

        # cycle number and experiment id
        if ((RundexSettings.CYCLE_JSON_KEY in json_data)
                and (RundexSettings.EXPERIMENT_JSON_KEY in json_data)):
            cycle = json_data[RundexSettings.CYCLE_JSON_KEY]
            exp = json_data[RundexSettings.EXPERIMENT_JSON_KEY]
            drill.setCycleAndExperiment(cycle, exp)

        # visual setings
        if RundexSettings.VISUAL_SETTINGS_JSON_KEY in json_data:
            visualSettings = json_data[RundexSettings.VISUAL_SETTINGS_JSON_KEY]
        else:
            visualSettings = None
        drill.setVisualSettings(visualSettings)

        # global settings
        if (RundexSettings.SETTINGS_JSON_KEY in json_data):
            settings = json_data[RundexSettings.SETTINGS_JSON_KEY]
            drill.setSettings(settings)
        else:
            logger.warning("No global settings found when importing {0}. "
                           "Default settings will be used."
                           .format(self._filename))

        # export settings
        if (RundexSettings.EXPORT_JSON_KEY in json_data):
            algos = json_data[RundexSettings.EXPORT_JSON_KEY]
            exportModel = drill.getExportModel()
            if exportModel:
                for algo in algos:
                    exportModel.activateAlgorithm(algo)

        # samples
        if ((RundexSettings.SAMPLES_JSON_KEY in json_data)
                and (json_data[RundexSettings.SAMPLES_JSON_KEY])):
            for sampleJson in json_data[RundexSettings.SAMPLES_JSON_KEY]:
                # for backward compatibility
                if "CustomOptions" in sampleJson:
                    sampleJson.update(sampleJson["CustomOptions"])
                    del sampleJson["CustomOptions"]
                sample = DrillSample()
                sample.setParameters(sampleJson)
                drill.addSample(-1, sample)
        else:
            logger.warning("No sample found when importing {0}."
                           .format(self._filename))

        # groups
        if "SamplesGroups" in json_data and json_data["SamplesGroups"]:
            drill.setSamplesGroups(json_data["SamplesGroups"])
        if "MasterSamples" in json_data and json_data["MasterSamples"]:
            drill.setMasterSamples(json_data["MasterSamples"])

    def save(self):
        """
        Export the data from the model.
        """
        drill = self._drillModel
        json_data = dict()
        json_data[RundexSettings.INSTRUMENT_JSON_KEY] = drill.getInstrument()
        json_data[RundexSettings.MODE_JSON_KEY] = drill.getAcquisitionMode()

        # experiment
        cycle, exp = drill.getCycleAndExperiment()
        if cycle:
            json_data[RundexSettings.CYCLE_JSON_KEY] = cycle
        if exp:
            json_data[RundexSettings.EXPERIMENT_JSON_KEY] = exp

        # visual setings
        visualSettings = drill.getVisualSettings()
        if visualSettings:
            json_data[RundexSettings.VISUAL_SETTINGS_JSON_KEY] = visualSettings

        # global settings
        settings = drill.getSettings()
        if settings:
            json_data[RundexSettings.SETTINGS_JSON_KEY] = settings

        # export settings
        exportModel = drill.getExportModel()
        if exportModel:
            algos = [algo for algo in exportModel.getAlgorithms()
                     if exportModel.isAlgorithmActivated(algo)]
            json_data[RundexSettings.EXPORT_JSON_KEY] = algos

        # samples
        samples = drill.getSamples()
        if samples:
            json_data[RundexSettings.SAMPLES_JSON_KEY] = list()
            for sample in samples:
                json_data[RundexSettings.SAMPLES_JSON_KEY].append(
                        sample.getParameters())

        # groups
        groups = drill.getSamplesGroups()
        if groups:
            json_data["SamplesGroups"] = dict()
        for k,v in groups.items():
            json_data["SamplesGroups"][k] = list(v)
        masters = drill.getMasterSamples()
        if masters:
            json_data["MasterSamples"] = masters

        with open(self._filename, 'w') as json_file:
            json.dump(json_data, json_file, indent=4)
