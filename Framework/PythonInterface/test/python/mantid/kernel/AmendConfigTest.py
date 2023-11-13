# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantid.kernel import ConfigService, amend_config


class AmendConfigTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.conf = ConfigService.Instance()
        if cls.conf.getFacility().name() == "ANSTO":
            cls.temp_facility = "SINQ"
            cls.temp_instrument = "HRPT"
        else:
            cls.temp_facility = "ANSTO"
            cls.temp_instrument = "EMUau"
        cls.current_facility = cls.conf.getFacility().name()
        cls.current_instrument = cls.conf.getInstrument().name()
        cls.current_dirs = cls.conf["datasearch.directories"]

    def test_set_facility(cls):
        with amend_config(facility=cls.temp_facility):
            cls.assertEqual(cls.temp_facility, cls.conf.getFacility().name())

        cls.assertEqual(cls.current_facility, cls.conf.getFacility().name())

    def test_set_instrument(cls):
        with amend_config(facility=cls.temp_facility, instrument=cls.temp_instrument):
            cls.assertEqual(cls.temp_instrument, cls.conf.getInstrument().name())

        cls.assertEqual(cls.current_instrument, cls.conf.getInstrument().name())

    def test_set_data_dir(cls):
        temp_dir = "/test/dir/for/testing/"
        temp_dirs = temp_dir + ";" + cls.current_dirs

        with amend_config(data_dir=temp_dir):
            cls.assertEqual(temp_dirs, cls.conf["datasearch.directories"])

        cls.assertEqual(cls.current_dirs, cls.conf["datasearch.directories"])

        with amend_config(data_dir=temp_dir, prepend_datadir=False):
            cls.assertEqual(temp_dir, cls.conf["datasearch.directories"])

        cls.assertEqual(cls.current_dirs, cls.conf["datasearch.directories"])

    def test_with_kwargs(cls):
        temp_config = {"network.default.timeout": "90", "plots.ShowLegend": "Off", "plots.images.Colormap": "blues"}
        backup = {}
        for key in temp_config.keys():
            backup[key] = cls.conf[key]
        with amend_config(**temp_config):
            for key in temp_config.keys():
                cls.assertEqual(temp_config[key], cls.conf[key])
        for key in backup.keys():
            cls.assertEqual(backup[key], cls.conf[key])


if __name__ == "__main__":
    unittest.main()
