# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +


class MonitorID(object):
    def __init__(self, monitor_spec_num, monitor_name=None):
        self.monitor_name = monitor_name
        self.monitor_spec_num = monitor_spec_num

    def __eq__(self, o):  # -> bool:
        if isinstance(o, MonitorID):
            return self.monitor_spec_num == o.monitor_spec_num and self.monitor_name == o.monitor_name
        return False
