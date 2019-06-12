# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, unicode_literals)

from Muon.GUI.Common.contexts.muon_context import MuonContext
from Muon.GUI.Common.contexts.muon_data_context import MuonDataContext
from Muon.GUI.Common.contexts.muon_group_pair_context import MuonGroupPairContext
from Muon.GUI.Common.contexts.muon_gui_context import MuonGuiContext
from Muon.GUI.Common.muon_load_data import MuonLoadData
from Muon.GUI.Common.contexts.phase_table_context import PhaseTableContext
from Muon.GUI.Common.contexts.fitting_context import FittingContext
from mantid.py3compat import mock


def setup_context_for_tests(parent_object):
    parent_object.loaded_data = MuonLoadData()
    parent_object.loaded_data.get_main_field_direction = mock.MagicMock(return_value='transverse')
    parent_object.data_context = MuonDataContext(load_data=parent_object.loaded_data)
    parent_object.gui_context = MuonGuiContext()
    parent_object.group_context = MuonGroupPairContext(parent_object.data_context.check_group_contains_valid_detectors)
    parent_object.phase_table_context = PhaseTableContext()
    parent_object.fitting_context = FittingContext()
    parent_object.context = MuonContext(muon_data_context=parent_object.data_context, muon_group_context=parent_object.group_context,
                                        muon_gui_context=parent_object.gui_context, muon_phase_context=parent_object.phase_table_context,
                                        fitting_context=parent_object.fitting_context)


def setup_context():
    loaded_data = MuonLoadData()
    loaded_data.get_main_field_direction = mock.MagicMock(return_value='transverse')
    data_context = MuonDataContext(load_data=loaded_data)
    gui_context = MuonGuiContext()
    group_context = MuonGroupPairContext(data_context.check_group_contains_valid_detectors)
    phase_table_context = PhaseTableContext()
    fitting_context = FittingContext()
    return MuonContext(muon_data_context=data_context,
                       muon_group_context=group_context,
                       muon_gui_context=gui_context,
                       muon_phase_context=phase_table_context,
                       fitting_context=fitting_context)
