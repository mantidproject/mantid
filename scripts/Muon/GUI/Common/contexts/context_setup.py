from Muon.GUI.Common.contexts.muon_context import MuonContext
from Muon.GUI.Common.contexts.muon_data_context import MuonDataContext
from Muon.GUI.Common.contexts.muon_group_pair_context import MuonGroupPairContext
from Muon.GUI.Common.contexts.muon_gui_context import MuonGuiContext
from Muon.GUI.Common.muon_load_data import MuonLoadData
from mantid.py3compat import mock


def setup_context_for_tests(parent_object):
    parent_object.loaded_data = MuonLoadData()
    parent_object.loaded_data.get_main_field_direction = mock.MagicMock(return_value='transverse')
    parent_object.data_context = MuonDataContext(parent_object.loaded_data)
    parent_object.gui_context = MuonGuiContext()
    parent_object.group_context = MuonGroupPairContext(parent_object.data_context.check_group_contains_valid_detectors)
    parent_object.context = MuonContext(muon_data_context=parent_object.data_context, muon_group_context=parent_object.group_context,
                                        muon_gui_context=parent_object.gui_context)
