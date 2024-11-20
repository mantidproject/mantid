# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from unittest.mock import MagicMock


class MockConfigService:
    def __init__(self):
        self.setString = MagicMock()
        self.getString = MagicMock()


BASE_CLASS_CONFIG_SERVICE_PATCH_PATH = "workbench.widgets.settings.base_classes.config_settings_changes_model.ConfigService"
