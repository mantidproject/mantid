# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.kernel import (Direction, Property)
from mantid.api import (DataProcessorAlgorithm, AlgorithmFactory, MatrixWorkspaceProperty, PropertyMode)
from sans.common.enums import (SANSFacility, OutputMode)
from collections import namedtuple
from sans.gui_logic.presenter.property_manager_service import PropertyManagerService
from sans.sans_batch import SANSBatchReduction

# ----------------------------------------------------------------------------------------------------------------------
# Globals
# ----------------------------------------------------------------------------------------------------------------------
SANS_DUMMY_INPUT_ALGORITHM_PROPERTY_NAME = '__sans_dummy_gui_workspace'
SANS_DUMMY_OUTPUT_ALGORITHM_PROPERTY_NAME = '__sans_dummy_gui_workspace'


# ----------------------------------------------------------------------------------------------------------------------
# Set up the white list and black list properties of the data algorithm
# ----------------------------------------------------------------------------------------------------------------------
algorithm_list_entry = namedtuple('algorithm_list_entry', 'column_name, algorithm_property, description, '
                                                          'show_value, default, prefix, property_type')


def create_option_column_properties():
    """
    Adds a new property which is meant for the Options column.

    This column should correspond to features in our settings section. We need to parse the entries before the
    runs are processed in order to account for the settings in the Options column in the state creation.

    Important note: If you add it here then you have to add it to the parsing logic, else nothing will happen with it.
                    The important bit to edit is in gui_state_director. There the set properties are parsed.
    """
    props = [algorithm_list_entry(column_name="",
                                  algorithm_property="WavelengthMin",
                                  description='The min value of the wavelength when converting from TOF.',
                                  show_value=True,
                                  default='',
                                  prefix='',
                                  property_type=float),
             algorithm_list_entry(column_name="",
                                  algorithm_property="WavelengthMax",
                                  description='The max value of the wavelength when converting from TOF.',
                                  show_value=True,
                                  default='',
                                  prefix='',
                                  property_type=float),
             algorithm_list_entry(column_name="",
                                  algorithm_property="EventSlices",
                                  description='The event slices to reduce. The format is the same as for the event slices'
                                              ' box in settings, however if a comma separated list is given '
                                              'it must be enclosed in quotes',
                                  show_value=True,
                                  default='',
                                  prefix='',
                                  property_type=str)
             ]
    return props


def create_properties(show_periods=True):
    if show_periods:
        properties = [algorithm_list_entry(column_name="SampleScatter",
                                           algorithm_property="SampleScatter",
                                           description='The run number of the scatter sample',
                                           show_value=False,
                                           default='',
                                           prefix='',
                                           property_type=str),
                      algorithm_list_entry(column_name="ssp",
                                           algorithm_property="SampleScatterPeriod",
                                           description='The sample scatter period',
                                           show_value=False,
                                           default='',
                                           prefix='',
                                           property_type=str),
                      algorithm_list_entry(column_name="SampleTrans",
                                           algorithm_property="SampleTransmission",
                                           description='The run number of the transmission sample',
                                           show_value=False,
                                           default='',
                                           prefix='',
                                           property_type=str),
                      algorithm_list_entry(column_name="stp",
                                           algorithm_property="SampleTransmissionPeriod",
                                           description='The sample transmission period',
                                           show_value=False,
                                           default='',
                                           prefix='',
                                           property_type=str),
                      algorithm_list_entry(column_name="SampleDirect",
                                           algorithm_property="SampleDirect",
                                           description='The run number of the direct sample',
                                           show_value=False,
                                           default='',
                                           prefix='',
                                           property_type=str),
                      algorithm_list_entry(column_name="sdp",
                                           algorithm_property="SampleDirectPeriod",
                                           description='The sample direct period',
                                           show_value=False,
                                           default='',
                                           prefix='',
                                           property_type=str),
                      algorithm_list_entry(column_name="CanScatter",
                                           algorithm_property="CanScatter",
                                           description='The run number of the scatter can',
                                           show_value=False,
                                           default='',
                                           prefix='',
                                           property_type=str),
                      algorithm_list_entry(column_name="csp",
                                           algorithm_property="CanScatterPeriod",
                                           description='The can scatter period',
                                           show_value=False,
                                           default='',
                                           prefix='',
                                           property_type=str),
                      algorithm_list_entry(column_name="CanTrans",
                                           algorithm_property="CanTransmission",
                                           description='The run number of the transmission can',
                                           show_value=False,
                                           default='',
                                           prefix='',
                                           property_type=str),
                      algorithm_list_entry(column_name="ctp",
                                           algorithm_property="CanTransmissionPeriod",
                                           description='The can transmission period',
                                           show_value=False,
                                           default='',
                                           prefix='',
                                           property_type=str),
                      algorithm_list_entry(column_name="CanDirect",
                                           algorithm_property="CanDirect",
                                           description='The run number of the direct can',
                                           show_value=False,
                                           default='',
                                           prefix='',
                                           property_type=str),
                      algorithm_list_entry(column_name="cdp",
                                           algorithm_property="CanDirectPeriod",
                                           description='The can direct period',
                                           show_value=False,
                                           default='',
                                           prefix='',
                                           property_type=str),
                      algorithm_list_entry(column_name="",
                                           algorithm_property="UseOptimizations",
                                           description='If optimizations should be used.',
                                           show_value=False,
                                           default=False,
                                           prefix='',
                                           property_type=bool),
                      algorithm_list_entry(column_name="",
                                           algorithm_property="PlotResults",
                                           description='If results should be plotted.',
                                           show_value=False,
                                           default=False,
                                           prefix='',
                                           property_type=bool),
                      algorithm_list_entry(column_name="OutputName",
                                           algorithm_property="OutputName",
                                           description='An optional custom output workspace name.',
                                           show_value=False,
                                           default='',
                                           prefix='',
                                           property_type=str),
                      algorithm_list_entry(column_name="User File",
                                           algorithm_property="UserFile",
                                           description=('The user file to use, this will override GUI changes for this row.'
                                                        ' If left unspecified default will be used'),
                                           show_value=False,
                                           default="",
                                           prefix='',
                                           property_type=str),
                      algorithm_list_entry(column_name="Sample Thickness",
                                           algorithm_property="SampleThickness",
                                           description=('The sample thickness from the user file'),
                                           show_value=False,
                                           default='',
                                           prefix='',
                                           property_type=str),
                      algorithm_list_entry(column_name="",
                                           algorithm_property="RowIndex",
                                           description='The row index (which is automatically populated by the GUI)',
                                           show_value=False,
                                           default=Property.EMPTY_INT,
                                           prefix='',
                                           property_type=int),
                      algorithm_list_entry(column_name="",
                                           algorithm_property="OutputMode",
                                           description='The output mode.',
                                           show_value=False,
                                           default=OutputMode.PublishToADS.name,
                                           prefix='',
                                           property_type=bool),
                      algorithm_list_entry(column_name="",
                                           algorithm_property="OutputGraph",
                                           description='The name of the graph to output to.',
                                           show_value=False,
                                           default='',
                                           prefix='',
                                           property_type=str)
                      ]
    else:
        properties = [algorithm_list_entry(column_name="SampleScatter",
                                           algorithm_property="SampleScatter",
                                           description='The run number of the scatter sample',
                                           show_value=False,
                                           default='',
                                           prefix='',
                                           property_type=str),
                      algorithm_list_entry(column_name="SampleTrans",
                                           algorithm_property="SampleTransmission",
                                           description='The run number of the transmission sample',
                                           show_value=False,
                                           default='',
                                           prefix='',
                                           property_type=str),
                      algorithm_list_entry(column_name="SampleDirect",
                                           algorithm_property="SampleDirect",
                                           description='The run number of the direct sample',
                                           show_value=False,
                                           default='',
                                           prefix='',
                                           property_type=str),
                      algorithm_list_entry(column_name="CanScatter",
                                           algorithm_property="CanScatter",
                                           description='The run number of the scatter can',
                                           show_value=False,
                                           default='',
                                           prefix='',
                                           property_type=str),
                      algorithm_list_entry(column_name="CanTrans",
                                           algorithm_property="CanTransmission",
                                           description='The run number of the transmission can',
                                           show_value=False,
                                           default='',
                                           prefix='',
                                           property_type=str),
                      algorithm_list_entry(column_name="CanDirect",
                                           algorithm_property="CanDirect",
                                           description='The run number of the direct can',
                                           show_value=False,
                                           default='',
                                           prefix='',
                                           property_type=str),
                      algorithm_list_entry(column_name="",
                                           algorithm_property="UseOptimizations",
                                           description='If optimizations should be used.',
                                           show_value=False,
                                           default=False,
                                           prefix='',
                                           property_type=bool),
                      algorithm_list_entry(column_name="",
                                           algorithm_property="PlotResults",
                                           description='If results should be plotted.',
                                           show_value=False,
                                           default=False,
                                           prefix='',
                                           property_type=bool),
                      algorithm_list_entry(column_name="OutputName",
                                           algorithm_property="OutputName",
                                           description='An optional custom output workspace name.',
                                           show_value=False,
                                           default='',
                                           prefix='',
                                           property_type=str),
                      algorithm_list_entry(column_name="User File",
                                           algorithm_property="UserFile",
                                           description=('The user file to use, this will override GUI changes for this row.'
                                                        ' If left unspecified default will be used'),
                                           show_value=False,
                                           default="",
                                           prefix='',
                                           property_type=str),
                      algorithm_list_entry(column_name="Sample Thickness",
                                           algorithm_property="SampleThickness",
                                           description=('The sample thickness from the user file'),
                                           show_value=False,
                                           default='',
                                           prefix='',
                                           property_type=str),
                      algorithm_list_entry(column_name="",
                                           algorithm_property="RowIndex",
                                           description='The row index (which is automatically populated by the GUI)',
                                           show_value=False,
                                           default=Property.EMPTY_INT,
                                           prefix='',
                                           property_type=int),
                      algorithm_list_entry(column_name="",
                                           algorithm_property="OutputMode",
                                           description='The output mode.',
                                           show_value=False,
                                           default=OutputMode.PublishToADS.name,
                                           prefix='',
                                           property_type=bool),
                      algorithm_list_entry(column_name="",
                                           algorithm_property="OutputGraph",
                                           description='The name of the graph to output to.',
                                           show_value=False,
                                           default='',
                                           prefix='',
                                           property_type=str)
                      ]
    return properties


def get_white_list(show_periods=True):
    return create_properties(show_periods=show_periods)


def get_black_list(show_periods=True):
    black_list = "InputWorkspace,OutputWorkspace,"
    properties = create_properties(show_periods=show_periods)
    for prop in properties:
        if not prop.show_value:
            black_list += prop.algorithm_property
            black_list += ","
    return black_list


def get_gui_algorithm_name(facility):
    if facility is SANSFacility.ISIS:
        algorithm_name = "SANSGuiDataProcessorAlgorithm"
        AlgorithmFactory.subscribe(SANSGuiDataProcessorAlgorithm)
    else:
        raise RuntimeError("The facility is currently not supported")
    return algorithm_name


class SANSGuiDataProcessorAlgorithm(DataProcessorAlgorithm):
    def category(self):
        return 'SANS\\Gui'

    def summary(self):
        return 'Dynamic SANS Gui algorithm.'

    def PyInit(self):
        # ------------------------------------------------------------
        # Dummy workspace properties.
        # ------------------------------------------------------------
        self.declareProperty(MatrixWorkspaceProperty("InputWorkspace", SANS_DUMMY_INPUT_ALGORITHM_PROPERTY_NAME,
                                                     optional=PropertyMode.Optional, direction=Direction.Input),
                             doc='The input workspace (which is not used)')

        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspace", SANS_DUMMY_OUTPUT_ALGORITHM_PROPERTY_NAME,
                                                     optional=PropertyMode.Optional, direction=Direction.Output),
                             doc='The output workspace (which is not used)')

        # ------------------------------------------------------------
        # Create the properties
        # ------------------------------------------------------------
        properties = create_properties()
        for prop in properties:
            self.declareProperty(prop.algorithm_property, defaultValue=prop.default,
                                 direction=Direction.Input, doc=prop.description)

        # ------------------------------------------------------------
        # Add properties which will show up in the options column
        # ------------------------------------------------------------
        properties = create_option_column_properties()
        for prop in properties:
            self.declareProperty(prop.algorithm_property, defaultValue=prop.default,
                                 direction=Direction.Input, doc=prop.description)

    def PyExec(self):
        # 1. Get the index of the batch reduction
        index = self.getProperty("RowIndex").value

        if index == Property.EMPTY_INT:
            return

        # 2. Get the state for the index from the PropertyManagerDataService
        property_manager_service = PropertyManagerService()
        state = property_manager_service.get_single_state_from_pmds(index_to_retrieve=index)
        # 3. Get some global settings
        use_optimizations = self.getProperty("UseOptimizations").value
        output_mode_as_string = self.getProperty("OutputMode").value
        output_mode = OutputMode[output_mode_as_string]
        plot_results = self.getProperty('PlotResults').value
        output_graph = self.getProperty('OutputGraph').value

        # 3. Run the sans_batch script
        sans_batch = SANSBatchReduction()
        sans_batch(states=state, use_optimizations=use_optimizations, output_mode=output_mode, plot_results=plot_results
                   , output_graph=output_graph)
