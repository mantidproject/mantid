import json
from SANSStateBase import (SANSStateBase, FloatParameter, DictParameter,
                           convert_state_to_dict, set_state_from_property_manager, sans_parameters)


# ------------------------------------------------
# SANSStateData
# ------------------------------------------------
class SANSStateMoveWorkspace(object):
    pass


class SANSStateMoveWorkspaceFactory(object):
    pass


# -----------------------------------------------
#  SANSStateMoveWorkspace Setup for the different machines
# -----------------------------------------------
@sans_parameters
class SANSStateMoveWorkspaceISIS(SANSStateBase, SANSStateMoveWorkspace):
    x_translation_correction = FloatParameter()
    y_translation_correction = FloatParameter()
    z_translation_correction = FloatParameter()

    def __init__(self):
        super(SANSStateMoveWorkspaceISIS, self).__init__()
        # Translation correction
        self.x_translation_correction = 0.0
        self.y_translation_correction = 0.0
        self.z_translation_correction = 0.0


class SANSStateMoveWorkspaceLOQ(SANSStateMoveWorkspaceISIS):
    monitor_names = DictParameter()
    center_position = FloatParameter()

    def __init__(self):
        super(SANSStateMoveWorkspaceLOQ, self).__init__()
        # Set the monitor names
        self.monitor_names = {1: 'monitor1',
                              2: 'monitor2',
                              3: 'monitor3',
                              4: 'monitor4'}

        # Set the center_position in meter
        self.center_position = 317.5 / 1000.

    @property
    def property_manager(self):
        return convert_state_to_dict(self)

    @property_manager.setter
    def property_manager(self, value):
        set_state_from_property_manager(self, value)

    def validate(self):
        pass


# -----------------------------------------------
# SANSStateData setup for other facilities/techniques/scenarios.
# Needs to derive from SANSStateData and SANSStateBase and fulfill its contract.
# -----------------------------------------------
