# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init
from mantid.simpleapi import *
from mantid.api import *
from mantid.kernel import *
from mantid import config
import math


def _get_instrument_property_list(instrument, property_name):
    """
    Gets a list of properties from an instrument string property.

    @param instrument Instrument object
    @param property_name Name of property
    @return A list of string values
    """

    raw_property_list = instrument.getStringParameter(property_name)
    if raw_property_list is None or len(raw_property_list) == 0:
        raise RuntimeError("Got empty list for parameter %s" % property_name)

    property_list = raw_property_list[0].split(",")

    return property_list


class IndirectTransmission(PythonAlgorithm):
    def category(self):
        return "Workflow\\MIDAS"

    def summary(self):
        return "Calculates the scattering & transmission for Indirect Geometry spectrometers."

    def PyInit(self):
        self.declareProperty(
            name="Instrument",
            defaultValue="IRIS",
            validator=StringListValidator(["IRIS", "OSIRIS", "TOSCA", "BASIS", "VISION", "IN16B"]),
            doc="Instrument",
        )

        self.declareProperty(
            name="Analyser",
            defaultValue="graphite",
            validator=StringListValidator(["graphite", "mica", "fmica", "silicon"]),
            doc="Analyser",
        )

        self.declareProperty(
            name="Reflection", defaultValue="002", validator=StringListValidator(["002", "004", "006", "111", "311"]), doc="Reflection"
        )

        self.declareProperty(name="ChemicalFormula", defaultValue="", validator=StringMandatoryValidator(), doc="Sample chemical formula")

        self.declareProperty(
            name="DensityType",
            defaultValue="Mass Density",
            validator=StringListValidator(["Mass Density", "Number Density"]),
            doc="Use of Mass density or Number density",
        )

        self.declareProperty(
            name="Density", defaultValue=0.1, doc="Mass density (g/cm^3) or Number density (atoms/Angstrom^3). Default=0.1"
        )

        self.declareProperty(name="Thickness", defaultValue=0.1, doc="Sample thickness (cm). Default=0.1")

        self.declareProperty(WorkspaceProperty("OutputWorkspace", "", Direction.Output), doc="The name of the output workspace.")

    # pylint: disable=too-many-locals
    def PyExec(self):
        instrument_name = self.getPropertyValue("Instrument")
        analyser = self.getPropertyValue("Analyser")
        reflection = self.getPropertyValue("Reflection")
        formula = self.getPropertyValue("ChemicalFormula")
        densityType = self.getPropertyValue("DensityType")
        density = self.getProperty("Density").value
        thickness = self.getPropertyValue("Thickness")

        # Create an empty instrument workspace
        workspace = "__empty_" + instrument_name
        CreateSimulationWorkspace(OutputWorkspace=workspace, Instrument=instrument_name, BinParams="0,0.5,1")

        # Do some validation on the analyser and reflection
        instrument = mtd[workspace].getInstrument()
        valid_analysers = _get_instrument_property_list(instrument, "analysers")
        logger.debug("Valid analysers for instrument %s: %s" % (instrument_name, str(valid_analysers)))

        # Check the analyser is valid for the instrument
        if analyser not in valid_analysers:
            # Remove idf/ipf workspace
            DeleteWorkspace(workspace)
            raise RuntimeError("Analyser %s not valid for instrument %s" % (analyser, instrument_name))

        else:
            # If the analyser was valid then we can check the reflection
            reflections_param_name = "refl-%s" % analyser
            valid_reflections = _get_instrument_property_list(instrument, reflections_param_name)
            logger.debug("Valid reflections for analyser %s: %s" % (analyser, str(valid_reflections)))

            if reflection not in valid_reflections:
                # Remove idf/ipf workspace
                DeleteWorkspace(workspace)
                raise RuntimeError("Reflection %s not valid for analyser %s on instrument %s" % (reflection, analyser, instrument_name))

        # Load instrument parameter file
        idf_directory = config["instrumentDefinition.directory"]
        name_stem = instrument_name + "_" + analyser + "_" + reflection
        ipf_filename = idf_directory + name_stem + "_Parameters.xml"
        LoadParameterFile(Workspace=workspace, Filename=ipf_filename)

        # Get efixed value
        efixed = self._get_efixed(workspace)

        logger.notice("Analyser : " + analyser + reflection + " with energy = " + str(efixed))

        if densityType == "Mass Density":
            SetSampleMaterial(InputWorkspace=workspace, ChemicalFormula=formula, SampleMassDensity=density)
        else:
            SetSampleMaterial(InputWorkspace=workspace, ChemicalFormula=formula, SampleNumberDensity=density)

        # Elastic wavelength
        wave = 1.8 * math.sqrt(25.2429 / efixed)

        material = mtd[str(workspace)].sample().getMaterial()
        number_density = material.numberDensityEffective
        absorption_x_section = material.absorbXSection(wave)
        coherent_x_section = material.cohScatterXSection()
        incoherent_x_section = material.incohScatterXSection()
        scattering_s_section = incoherent_x_section + coherent_x_section

        thickness = float(thickness)

        total_x_section = absorption_x_section + scattering_s_section

        transmission = math.exp(-number_density * total_x_section * thickness)
        scattering = 1.0 - math.exp(-number_density * scattering_s_section * thickness)

        # Create table workspace to store calculations
        table_ws = self.getPropertyValue("OutputWorkspace")
        table_ws = CreateEmptyTableWorkspace(OutputWorkspace=table_ws)
        table_ws.addColumn("str", "Name")
        table_ws.addColumn("double", "Value")

        # Names for each of the output values
        output_names = [
            "Wavelength",
            "Absorption Xsection",
            "Coherent Xsection",
            "Incoherent Xsection",
            "Total scattering Xsection",
            "Number density",
            "Thickness",
            "Transmission (abs+scatt)",
            "Total scattering",
        ]

        # List of the calculated values
        output_values = [
            wave,
            absorption_x_section,
            coherent_x_section,
            incoherent_x_section,
            scattering_s_section,
            number_density,
            thickness,
            transmission,
            scattering,
        ]

        # Build table of values
        for data in zip(output_names, output_values):
            table_ws.addRow(list(data))
            logger.information(": ".join([str(d) for d in data]))

        # Remove idf/ipf workspace
        DeleteWorkspace(workspace)

        self.setProperty("OutputWorkspace", table_ws)

    def _get_efixed(self, workspace):
        """
        Gets an efixed value form a workspace.

        @param workspace Name of workspace to extract from
        @return Fixed energy value
        """
        from IndirectCommon import get_efixed

        try:
            # Try to get efixed from the parameters first
            efixed = get_efixed(workspace)

        except ValueError:
            # If that fails then get it by taking from group of all detectors
            wsHandle = mtd[workspace]
            spectra_list = list(range(0, wsHandle.getNumberHistograms()))
            GroupDetectors(InputWorkspace=workspace, OutputWorkspace=workspace, SpectraList=spectra_list)
            wsHandle = mtd[workspace]
            det = wsHandle.getDetector(0)
            efixed = wsHandle.getEFixed(det.getID())

        return efixed


# Register algorithm with Mantid
AlgorithmFactory.subscribe(IndirectTransmission)
