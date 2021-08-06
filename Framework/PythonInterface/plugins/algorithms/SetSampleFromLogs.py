# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import (AlgorithmFactory, DistributedDataProcessorAlgorithm)
from mantid.kernel import (ConfigService)
from mantid import _kernel as mtd_kernel
from mantid.simpleapi import (SetSample)


PROPS_FOR_SETSAMPLE = ["InputWorkspace", "Geometry", "Material", "Environment", "ContainerGeometry", "ContainerMaterial"]


def _toDict(propertyManager):
    result = {}
    if propertyManager:
        for key in propertyManager.keys():
            result[key] = propertyManager[key].value
            if isinstance(result[key], mtd_kernel.std_vector_dbl):
                result[key] = list(result[key])
    return result


def _findKey(dictlike, *args):
    for name in args:
        if (name in dictlike) and bool(dictlike[name]):
            return name
    return ''  # indicates failure


def _hasValue(dictLike, key):  # TODO refactor into *args and return the one that exists or empty string
    return (key in dictLike) and bool(dictLike[key])


def _getLogValue(propertyManager, key):
    if not _hasValue(propertyManager, key):
        raise ValueError('Failed to find "{}" in logs'.format(key))
    return propertyManager[key].lastValue()


class SetSampleFromLogs(DistributedDataProcessorAlgorithm):
    def category(self):
        return "Sample"

    def seeAlso(self):
        return ["SetSample"]

    def name(self):
        return "SetSampleFromLogs"

    def summary(self):
        return "This algorithm looks through the logs to automatically determine the sample geometry and material"

    def PyInit(self):
        self.copyProperties("SetSample", PROPS_FOR_SETSAMPLE)
        self.declareProperty("FindGeometry", True,
                             "Whether to look for the 'Height' parameter in the logs")
        self.declareProperty("FindSample", True,
                             "Whether to look for the sample material in the logs")
        self.declareProperty("FindEnvironment", True,
                             "Whether to look for the sample container in the logs")

    def PyExec(self):
        wksp = self.getProperty("InputWorkspace").value
        geometry = _toDict(self.getProperty("Geometry").value)
        material = _toDict(self.getProperty("Material").value)
        environment = _toDict(self.getProperty("Environment").value)
        geometryContainer = _toDict(self.getProperty("ContainerGeometry").value)
        materialContainer = _toDict(self.getProperty("ContainerMaterial").value)
        findGeometry = self.getProperty("FindGeometry").value
        findSample = self.getProperty("FindSample").value
        findEnvironment = self.getProperty("FindEnvironment").value

        # get a convenient handle to the logs
        runObject = wksp.run()

        # get information for the material from the logs
        if findSample:
            if (not _hasValue(material, 'ChemicalFormula')) and _hasValue(runObject, "SampleFormula"):
                self.log().information("Looking for 'SampleFormula' in logs")
                material['ChemicalFormula'] = _getLogValue(runObject, 'SampleFormula').strip()
            if (not _hasValue(material, "SampleMassDensity")) and _hasValue(runObject, "SampleDensity"):
                self.log().information("Looking for 'SampleDensity', 'SampleMass', and 'SampleMassDensity' in logs")
                value = _getLogValue(runObject, 'SampleDensity')
                if value == 1.0 or value == 0.0:
                    material['Mass'] = _getLogValue(runObject, 'SampleMass')
                elif _hasValue(runObject, "SampleMass"):
                    material['SampleMassDensity'] = value
        self.log().information('MATERIAL: ' + str(material))

        # get height of sample from the logs
        # this assumes the shape has a "Height" property
        if findGeometry:
            instrEnum = ConfigService.getInstrument(wksp.getInstrument().getFullName())
            if not _hasValue(geometry, "Height"):
                heightInContainerNames = ['HeightInContainer']
                heightInContainerUnitsNames = ['HeightInContainerUnits']

                # determine "special" logs for SNS instruments
                if instrEnum.facility().name() == 'SNS':
                    beamline = ''
                    # TODO this would benefit from the beamline exposed through python
                    if instrEnum.name() == 'NOMAD':
                        beamline = 'BL1B'
                    elif instrEnum.name() == 'POWGEN':
                        beamline = 'BL11A'
                    else:
                        warningMsg = 'Do not know how to create lognames for "{}"'.format(instrEnum.name())
                        self.log().warn(warningMsg)
                    if beamline:
                        heightInContainerUnitsNames.append('{}:CS:ITEMS:HeightInContainerUnits'.format(beamline))
                        heightInContainerNames.append('{}:CS:ITEMS:HeightInContainer'.format(beamline))
                self.log().information("Looking for sample height from {} and units from {}".format(heightInContainerNames,
                                                                                                    heightInContainerUnitsNames))
                # Check units - SetSample expects cm
                unitsKey = _findKey(runObject, *heightInContainerUnitsNames)
                units = ''
                if unitsKey:
                    units = _getLogValue(runObject, unitsKey)
                    # not finding units will generate a warning below
                # create conversion factor into cm
                conversion = 1.0  # don't do any conversion
                if units == "cm":
                    conversion = 1.0
                elif units == "mm":
                    conversion = 0.1
                else:
                    warningMsg = "HeightInContainerUnits expects cm or mm;" + \
                                " specified units not recognized: {:s};".format(units) + \
                                " we will reply on user input for sample density information."
                    self.log().warn(warningMsg)

                # set the height
                heightKey = _findKey(runObject, *heightInContainerNames)
                if heightKey:
                    geometry['Height'] = conversion * _getLogValue(runObject, heightKey)
                else:
                    raise RuntimeError("Failed to determine the height from the logs")
        self.log().information('GEOMETRY (in cm): ' + str(geometry))

        # get the container from the logs
        if findEnvironment:
            if (not _hasValue(environment, "Container")) and _hasValue(runObject, "SampleContainer"):
                self.log().information('Looking for "SampleContainer" in logs')
                environment['Container'] = runObject['SampleContainer'].lastValue().replace(" ", "")

            if _hasValue(environment, "Container") and (not _hasValue(environment, "Name")):
                # set a default environment
                instrEnum = ConfigService.getInstrument(wksp.getInstrument().getFullName())
                if instrEnum.facility().name() == 'SNS':
                    environment['Name'] = 'InAir'

        self.log().information('ENVIRONMENT: ' + str(environment))

        # let SetSample generate errors if anything is wrong
        SetSample(InputWorkspace=wksp,
                  Material=material,
                  Geometry=geometry,
                  Environment=environment,
                  ContainerGeometry=geometryContainer,
                  ContainerMaterial=materialContainer)


# Register algorithm with Mantid.
AlgorithmFactory.subscribe(SetSampleFromLogs)
