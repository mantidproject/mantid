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
        raise RuntimeError('Got empty list for parameter %s' % property_name)

    property_list = raw_property_list[0].split(',')

    return property_list


class IndirectTransmission(PythonAlgorithm):

    def category(self):
        return "Workflow\\MIDAS;PythonAlgorithms"


    def summary(self):
        return "Calculates the scattering & transmission for Indirect Geometry spectrometers."


    def PyInit(self):
        self.declareProperty(name='Instrument', defaultValue='IRIS',
                             validator=StringListValidator(['IRIS', 'OSIRIS', 'BASIS', 'VISION']),
                             doc='Instrument')

        self.declareProperty(name='Analyser', defaultValue='graphite',
                             validator=StringListValidator(['graphite', 'mica', 'fmica', 'silicon']),
                             doc='Analyser')

        self.declareProperty(name='Reflection', defaultValue='002',
                             validator=StringListValidator(['002', '004', '006', '111']),
                             doc='Reflection')

        self.declareProperty(name='ChemicalFormula', defaultValue='', validator=StringMandatoryValidator(),
                             doc='Sample chemical formula')

        self.declareProperty(name='NumberDensity', defaultValue=0.1,
                             doc='Number denisty (atoms/Angstrom^3). Default=0.1')

        self.declareProperty(name='Thickness', defaultValue=0.1,
                             doc='Sample thickness (cm). Default=0.1')

        self.declareProperty(WorkspaceProperty('OutputWorkspace', "", Direction.Output),
                             doc="The name of the output workspace.")


    def PyExec(self):
        from IndirectCommon import StartTime, EndTime

        StartTime('IndirectTransmission')

        instrument_name = self.getPropertyValue('Instrument')
        analyser = self.getPropertyValue('Analyser')
        reflection = self.getPropertyValue('Reflection')
        formula = self.getPropertyValue('ChemicalFormula')
        density = self.getPropertyValue('NumberDensity')
        thickness = self.getPropertyValue('Thickness')

        # Create an empty instrument workspace
        workspace = self._create_instrument_workspace(instrument_name)

        # Do some validation on the analyser and reflection
        instrument = mtd[workspace].getInstrument()
        valid_analysers = _get_instrument_property_list(instrument, 'analysers')
        logger.debug('Valid analysers for instrument %s: %s' % (instrument_name, str(valid_analysers)))

        # Check the analyser is valid for the instrument
        if analyser not in valid_analysers:
            # Remove idf/ipf workspace
            DeleteWorkspace(workspace)
            raise RuntimeError('Analyser %s not valid for instrument %s' % (analyser, instrument_name))

        else:
            # If the analyser was valid then we can check the reflection
            reflections_param_name = 'refl-%s' % analyser
            valid_reflections = _get_instrument_property_list(instrument, reflections_param_name)
            logger.debug('Valid reflections for analyser %s: %s' % (analyser, str(valid_reflections)))

            if reflection not in valid_reflections:
                # Remove idf/ipf workspace
                DeleteWorkspace(workspace)
                raise RuntimeError('Reflection %s not valid for analyser %s on instrument %s' % (reflection, analyser, instrument_name))

        # Load instrument parameter file
        idf_directory = config['instrumentDefinition.directory']
        name_stem = instrument_name + '_' + analyser + '_' + reflection
        ipf_filename = idf_directory + name_stem + '_Parameters.xml'
        LoadParameterFile(Workspace=workspace, Filename=ipf_filename)

        # Get efixed value
        instrument = mtd[workspace].getInstrument()
        efixed = instrument.getNumberParameter('efixed-val')[0]

        logger.notice('Analyser : ' + analyser + reflection + ' with energy = ' + str(efixed))

        result = SetSampleMaterial(InputWorkspace=workspace, ChemicalFormula=formula)

        # Elastic wavelength
        wave = 1.8 * math.sqrt(25.2429 / efixed)

        absorption_x_section = result[5] * wave / 1.7982
        coherent_x_section = result[4]
        incoherent_x_section = result[3]
        scattering_s_section = incoherent_x_section + coherent_x_section

        thickness = float(thickness)
        density = float(density)

        total_x_section = absorption_x_section + scattering_s_section

        transmission = math.exp(-density * total_x_section * thickness)
        scattering = 1.0 - math.exp(-density * scattering_s_section * thickness)

        # Create table workspace to store calculations
        table_ws = self.getPropertyValue('OutputWorkspace')
        table_ws = CreateEmptyTableWorkspace(OutputWorkspace=table_ws)
        table_ws.addColumn("str", "Name")
        table_ws.addColumn("double", "Value")

        # Names for each of the output values
        output_names = ['Wavelength', 'Absorption Xsection', 'Coherent Xsection', 'Incoherent Xsection',
                        'Total scattering Xsection', 'Number density', 'Thickness', 'Transmission (abs+scatt)', 'Total scattering']

        # List of the calculated values
        output_values = [wave, absorption_x_section, coherent_x_section, incoherent_x_section,
                         scattering_s_section, density, thickness, transmission, scattering]

        # Build table of values
        for data in zip(output_names, output_values):
            table_ws.addRow(list(data))
            logger.information(': '.join(map(str, list(data))))

        # Remove idf/ipf workspace
        DeleteWorkspace(workspace)

        self.setProperty("OutputWorkspace", table_ws)

        EndTime('IndirectTransmission')


    def _create_instrument_workspace(self, instrument_name):
        """
        Creates a workspace with the most recent version of the given instrument attached to it.

        @param instrument_name Name of the instrument to load
        @return Name of the created workspace
        """

        # Get the filename for the most recent instrument defintion
        CreateSampleWorkspace(OutputWorkspace='__temp')
        idf_filename = mtd['__temp'].getInstrumentFilename(instrument_name)
        DeleteWorkspace('__temp')

        # Load instrument defintion file
        workspace = '__empty_' + instrument_name
        LoadEmptyInstrument(OutputWorkspace=workspace, Filename=idf_filename)

        return workspace


# Register algorithm with Mantid
AlgorithmFactory.subscribe(IndirectTransmission)
