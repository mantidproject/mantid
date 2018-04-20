from __future__ import (absolute_import, division, print_function)
import unittest
import mantid
from sans.test_helper.test_director import TestDirector
from sans.common.general_functions import create_unmanaged_algorithm
from sans.common.constants import EMPTY_NAME
from sans.common.enums import (DetectorType, DataType)


class SANSReductionCoreTest(unittest.TestCase):
    sample_workspace = None
    test_tof_min = 1000
    test_tof_max = 10000
    test_tof_width = 1000
    test_wav_min = 1.
    test_wav_max = 11.
    test_wav_width = 2.

    @staticmethod
    def _get_state():
        test_director = TestDirector()
        return test_director.construct()

    @staticmethod
    def _get_sample_monitor_data(value):
        create_name = "CreateSampleWorkspace"
        name = "test_workspace"
        create_options = {"OutputWorkspace": name,
                          "NumBanks": 0,
                          "NumMonitors": 8,
                          "XMin": SANSReductionCoreTest.test_tof_min,
                          "XMax": SANSReductionCoreTest.test_tof_max,
                          "BinWidth": SANSReductionCoreTest.test_tof_width}
        create_alg = create_unmanaged_algorithm(create_name, **create_options)
        create_alg.execute()
        monitor_workspace = create_alg.getProperty("OutputWorkspace").value
        for hist in range(monitor_workspace.getNumberHistograms()):
            data_y = monitor_workspace.dataY(hist)
            for index in range(len(data_y)):
                data_y[index] = value
            # This will be the background bin
            data_y[0] = 0.1
        return monitor_workspace

    @staticmethod
    def _get_sample_data():
        create_name = "CreateSampleWorkspace"
        name = "test_workspace"
        create_options = {"OutputWorkspace": name,
                          "NumBanks": 1,
                          "NumMonitors": 1,
                          "XMin": SANSReductionCoreTest.test_wav_min,
                          "XMax": SANSReductionCoreTest.test_wav_max,
                          "BinWidth": SANSReductionCoreTest.test_wav_width,
                          "XUnit": "Wavelength"}
        create_alg = create_unmanaged_algorithm(create_name, **create_options)
        create_alg.execute()
        return create_alg.getProperty("OutputWorkspace").value

    @staticmethod
    def _load_workspace(file_name):
        load_name = "Load"
        load_options = {"OutputWorkspace": EMPTY_NAME,
                        "Filename": file_name}
        load_alg = create_unmanaged_algorithm(load_name, **load_options)
        load_alg.execute()
        return load_alg.getProperty("OutputWorkspace").value

    @staticmethod
    def _clone_workspace(workspace):
        clone_name = "CloneWorkspace"
        clone_options = {"InputWorkspace": workspace,
                         "OutputWorkspace": EMPTY_NAME}
        clone_alg = create_unmanaged_algorithm(clone_name, **clone_options)
        clone_alg.execute()
        return clone_alg.getProperty("OutputWorkspace").value

    @staticmethod
    def _rebin_workspace(workspace):
        rebin_name = "Rebin"
        rebin_options = {"InputWorkspace": workspace,
                         "OutputWorkspace": EMPTY_NAME,
                         "Params": "{0}, {1}, {2}".format(SANSReductionCoreTest.test_tof_min,
                                                          SANSReductionCoreTest.test_tof_width,
                                                          SANSReductionCoreTest.test_tof_max)}
        rebin_alg = create_unmanaged_algorithm(rebin_name, **rebin_options)
        rebin_alg.execute()
        return rebin_alg.getProperty("OutputWorkspace").value

    @staticmethod
    def _get_trans_type_data(value):
        # Load the workspace
        if SANSReductionCoreTest.sample_workspace is None:
            SANSReductionCoreTest.sample_workspace = \
                SANSReductionCoreTest._load_workspace("SANS2D00022024")
        # Clone the workspace
        workspace = SANSReductionCoreTest._clone_workspace(
            SANSReductionCoreTest.sample_workspace)
        rebinned = SANSReductionCoreTest._rebin_workspace(workspace)
        # Set all entries to value
        for hist in range(rebinned.getNumberHistograms()):
            data_y = rebinned.dataY(hist)
            for index in range(len(data_y)):
                data_y[index] = value
            # This will be the background bin
            data_y[0] = 0.1
        return rebinned

    @staticmethod
    def _run_test(state, sample_data, sample_monitor_data, transmission_data, direct_data, is_lab=True, is_sample=True):
        reduction_name = "SANSReductionCore"
        reduction_options = {"SANSState": state,
                              "ScatterWorkspace": sample_data,
                              "ScatterMonitorWorkspace": sample_monitor_data,
                              "TransmissionWorkspace": transmission_data,
                              "DirectWorkspace": direct_data,
                              "OutputWorkspace": EMPTY_NAME,
                              "SumOfCounts": EMPTY_NAME,
                              "SumOfNormFactors": EMPTY_NAME,
                              "CalculatedTransmissionWorkspace": EMPTY_NAME,
                              "UnfittedTransmissionWorkspace": EMPTY_NAME}
        if is_sample:
            reduction_options.update({"DataType": DataType.to_string(DataType.Sample)})
        else:
            reduction_options.update({"DataType": DataType.to_string(DataType.Can)})
        if is_lab:
            reduction_options.update({"Component": DetectorType.to_string(DetectorType.LAB)})
        else:
            reduction_options.update({"Component": DetectorType.to_string(DetectorType.HAB)})

        reduction_alg = create_unmanaged_algorithm(reduction_name, **reduction_options)
        reduction_alg.execute()
        output = reduction_alg.getProperty("OutputWorkspace").value
        sum_of_counts = reduction_alg.getProperty("SumOfCounts").value
        sum_of_norms = reduction_alg.getProperty("SumOfNormFactors").value
        calculated_transmission = reduction_alg.getProperty("CalculatedTransmissionWorkspace").value
        unfitted_transmission = reduction_alg.getProperty("UnfittedTransmissionWorkspace").value
        return output, sum_of_counts, sum_of_norms,\
               calculated_transmission, unfitted_transmission

    def test_that_transmission_workspaces_are_produced_when_show_transmission(self):
        # Arrange
        state = SANSReductionCoreTest._get_state()
        state.adjustment.show_transmission = True
        serialized_state = state.property_manager
        sample_data = SANSReductionCoreTest._get_sample_data()
        sample_monitor_data = SANSReductionCoreTest._get_sample_monitor_data(3.)
        transmission_data = SANSReductionCoreTest._get_trans_type_data(1.)
        direct_data = SANSReductionCoreTest._get_trans_type_data(2.)
        import pydevd
        pydevd.settrace('localhost', port=5434, stdoutToServer=True, stderrToServer=True)
        # Act
        try:
            output, sum_of_counts, sum_of_norms,\
            calculated_transmission, unfitted_transmisison = \
                SANSReductionCoreTest._run_test(serialized_state, sample_data, sample_monitor_data,
                                                             transmission_data, direct_data)
            raised = False
            # We expect a wavelength adjustment workspace
            self.assertTrue(output)
            # We don't expect a pixel adjustment workspace since no files where specified
            self.assertFalse(sum_of_counts)
            # We expect a wavelength and pixel adjustment workspace since we set the flag to true and provided a
            # sample data set
            self.assertTrue(sum_of_norms)
            self.assertTrue(calculated_transmission)
            self.assertTrue(unfitted_transmisison)
        except:  # noqa
            raised = True
        self.assertFalse(raised)


if __name__ == '__main__':
    unittest.main()