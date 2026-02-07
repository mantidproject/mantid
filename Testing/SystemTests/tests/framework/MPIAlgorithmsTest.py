#!/usr/bin/env python
# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""
Standalone MPI tests for BroadcastWorkspace and GatherWorkspaces algorithms.
Run with: mpiexec -n 4 -l python mpitest.py
"""

from mantid.simpleapi import CreateWorkspace, GatherWorkspaces, BroadcastWorkspace, AppendSpectra, DeleteWorkspace, Scale, Load
from mantid.kernel import amend_config, Logger
from mpi4py import MPI
import numpy as np
import sys

logger = Logger("MPISystemTest")

comm = MPI.COMM_WORLD
rank = comm.Get_rank()
size = comm.Get_size()

# Test result tracking
passed_tests = 0
failed_tests = 0
test_names = []


def assert_equal(actual, expected, msg=""):
    """Simple assertion for equality"""
    if actual != expected:
        raise AssertionError(f"{msg}: Expected {expected}, got {actual}")


def run_test(test_name, test_func):
    """Run a test and track results"""
    global passed_tests, failed_tests, test_names

    if rank == 0:
        logger.information("=" * 60)
        logger.information(f"Running: {test_name}")
        logger.information("=" * 60)

    try:
        test_func()
        if rank == 0:
            logger.information(f"PASSED: {test_name}")
        passed_tests += 1
        test_names.append((test_name, "PASSED"))
    except Exception as e:
        if rank == 0:
            logger.information(f"FAILED: {test_name}")
            logger.information(f"Error: {e}")
        failed_tests += 1
        test_names.append((test_name, f"FAILED: {e}"))


def test_broadcast_basic():
    """Test basic broadcast functionality with shared X data"""
    # Only rank 0 creates the input workspace
    if rank == 0:
        x = np.linspace(0, 10, 101)
        y = np.tile(np.arange(100) % 10, 3)  # 3 spectra with repeating pattern
        e = np.ones(300) * 0.1

        input_ws = CreateWorkspace(DataX=x, DataY=y, DataE=e, NSpec=3, OutputWorkspace="broadcast_input")

        output_ws = BroadcastWorkspace(
            InputWorkspace=input_ws,
            BroadcasterRank=0,
            ChunkSize=0,  # Automatic
            OutputWorkspace="broadcast_output",
        )
    else:
        output_ws = BroadcastWorkspace(BroadcasterRank=0, ChunkSize=0, OutputWorkspace="broadcast_output")

    assert_equal(output_ws.getNumberHistograms(), 3, "Number of histograms")
    assert_equal(output_ws.blocksize(), 100, "Block size")

    x_expected = np.linspace(0, 10, 101)
    x_actual = output_ws.readX(0)
    np.testing.assert_array_almost_equal(x_actual, x_expected, err_msg="X data")

    y_spectrum0 = output_ws.readY(0)
    assert_equal(len(y_spectrum0), 100, "Y data length")
    np.testing.assert_array_equal(y_spectrum0[:10], np.arange(10), err_msg="Y data pattern")


def test_broadcast_chunked():
    """Test broadcast with explicit chunking using 10 spectra with sine waves at different phases"""
    if rank == 0:
        x = np.linspace(0, 100, 1001)
        y = np.concatenate([np.sin(x[:-1] + i) for i in range(10)])
        e = np.ones(10000)

        input_ws = CreateWorkspace(DataX=x, DataY=y, DataE=e, NSpec=10, OutputWorkspace="broadcast_chunked_input")

        output_ws = BroadcastWorkspace(InputWorkspace=input_ws, BroadcasterRank=0, ChunkSize=3, OutputWorkspace="broadcast_chunked_output")
    else:
        output_ws = BroadcastWorkspace(BroadcasterRank=0, ChunkSize=3, OutputWorkspace="broadcast_chunked_output")

    assert_equal(output_ws.getNumberHistograms(), 10, "Number of histograms")
    assert_equal(output_ws.blocksize(), 1000, "Block size")

    x_expected = np.linspace(0, 100, 1001)
    np.testing.assert_array_almost_equal(output_ws.readX(0), x_expected, err_msg="X data after chunked broadcast")

    y0 = output_ws.readY(0)
    y1 = output_ws.readY(1)
    assert np.sum(y0) != np.sum(y1), "Different spectra should have different Y sums"


def test_broadcast_nonshared_x():
    """Test broadcast with non-shared X data"""
    if rank == 0:
        ws1 = CreateWorkspace(
            DataX=np.array([0.0, 1.0, 2.0, 3.0, 4.0]),
            DataY=np.array([1.0, 2.0, 3.0, 4.0]),
            DataE=np.ones(4) * 0.1,
            NSpec=1,
            OutputWorkspace="temp1",
        )

        ws2 = CreateWorkspace(
            DataX=np.array([0.0, 2.0, 4.0, 6.0, 8.0]),  # Different X spacing
            DataY=np.array([5.0, 6.0, 7.0, 8.0]),
            DataE=np.ones(4) * 0.2,
            NSpec=1,
            OutputWorkspace="temp2",
        )

        input_ws = AppendSpectra(InputWorkspace1=ws1, InputWorkspace2=ws2, OutputWorkspace="broadcast_nonshared_input")

        DeleteWorkspace("temp1")
        DeleteWorkspace("temp2")

        output_ws = BroadcastWorkspace(
            InputWorkspace=input_ws, BroadcasterRank=0, ChunkSize=-1, OutputWorkspace="broadcast_nonshared_output"
        )
    else:
        output_ws = BroadcastWorkspace(BroadcasterRank=0, ChunkSize=-1, OutputWorkspace="broadcast_nonshared_output")

    assert_equal(output_ws.getNumberHistograms(), 2, "Number of histograms")

    # Verify different X arrays
    x0 = output_ws.readX(0)
    x1 = output_ws.readX(1)
    np.testing.assert_array_equal(x0, np.array([0.0, 1.0, 2.0, 3.0, 4.0]), err_msg="First X array")
    np.testing.assert_array_equal(x1, np.array([0.0, 2.0, 4.0, 6.0, 8.0]), err_msg="Second X array")

    np.testing.assert_array_equal(output_ws.readY(0), np.array([1.0, 2.0, 3.0, 4.0]), err_msg="First Y array")
    np.testing.assert_array_equal(output_ws.readY(1), np.array([5.0, 6.0, 7.0, 8.0]), err_msg="Second Y array")


def test_gather_append():
    """Test gathering workspaces with Append mode using rank-specific data: rank 0 -> all 1s, rank 1 -> all 2s..."""
    x = np.linspace(0, 10, 51)
    y = np.ones(100) * (rank + 1)
    e = np.ones(100) * 0.1

    input_ws = CreateWorkspace(DataX=x, DataY=y, DataE=e, NSpec=2, OutputWorkspace=f"gather_append_input_rank{rank}")

    if rank == 0:
        output_ws = GatherWorkspaces(
            InputWorkspace=input_ws, AccumulationMethod="Append", ChunkSize=0, OutputWorkspace="gather_append_output"
        )

        expected_spectra = 2 * size
        assert_equal(output_ws.getNumberHistograms(), expected_spectra, "Number of histograms")

        y0 = output_ws.readY(0)
        np.testing.assert_array_equal(y0, np.ones(50), err_msg="Rank 0 data")
        y2 = output_ws.readY(2)
        np.testing.assert_array_equal(y2, np.ones(50) * 2, err_msg="Rank 1 data")

        for r in range(size):
            spectrum_idx = r * 2
            y_data = output_ws.readY(spectrum_idx)
            np.testing.assert_array_equal(y_data, np.ones(50) * (r + 1), err_msg=f"Rank {r} data")
    else:
        GatherWorkspaces(InputWorkspace=input_ws, AccumulationMethod="Append", ChunkSize=0)


def test_gather_add():
    """Test gathering workspaces with Add mode Each rank creates a workspace with the same structure
    but rank 0 contributes 0s, rank 1 contributes 1s, rank 2 contributes 2s, etc."""
    x = np.linspace(0, 10, 51)
    y = np.ones(150) * rank
    e = np.ones(150) * 0.1

    input_ws = CreateWorkspace(DataX=x, DataY=y, DataE=e, NSpec=3, OutputWorkspace=f"gather_add_input_rank{rank}")

    if rank == 0:
        output_ws = GatherWorkspaces(InputWorkspace=input_ws, AccumulationMethod="Add", ChunkSize=1, OutputWorkspace="gather_add_output")

        assert_equal(output_ws.getNumberHistograms(), 3, "Number of histograms")

        expected_sum = sum(range(size))

        for spec_idx in range(3):
            y_data = output_ws.readY(spec_idx)
            np.testing.assert_array_equal(y_data, np.ones(50) * expected_sum, err_msg=f"Spectrum {spec_idx} summed values")

        expected_error = 0.1 * np.sqrt(size)
        for spec_idx in range(3):
            e_data = output_ws.readE(spec_idx)
            np.testing.assert_array_almost_equal(e_data, np.ones(50) * expected_error, err_msg=f"Spectrum {spec_idx} error propagation")
    else:
        GatherWorkspaces(InputWorkspace=input_ws, AccumulationMethod="Add", ChunkSize=1)


def test_gather_large_chunk():
    """Test gathering with large workspaces requiring chunking"""
    num_spectra = 20
    num_bins = 500
    x = np.linspace(0, 100, num_bins + 1)

    y = np.concatenate([np.sin(x[:-1] + rank + i) for i in range(num_spectra)])
    e = np.ones(num_bins * num_spectra) * 0.1

    input_ws = CreateWorkspace(DataX=x, DataY=y, DataE=e, NSpec=num_spectra, OutputWorkspace=f"gather_large_input_rank{rank}")

    if rank == 0:
        output_ws = GatherWorkspaces(
            InputWorkspace=input_ws, AccumulationMethod="Append", ChunkSize=5, OutputWorkspace="gather_large_output"
        )

        expected_spectra = 20 * size
        assert_equal(output_ws.getNumberHistograms(), expected_spectra, "Number of histograms")
        assert_equal(output_ws.blocksize(), 500, "Block size")

        y0_0 = output_ws.readY(0)
        expected_y0_0 = np.sin(x[:-1] + 0)
        np.testing.assert_array_almost_equal(y0_0, expected_y0_0, decimal=5, err_msg="Rank 0, spectrum 0 data")

        if size > 1:
            y1_0 = output_ws.readY(20)
            expected_y1_0 = np.sin(x[:-1] + 1)
            np.testing.assert_array_almost_equal(y1_0, expected_y1_0, decimal=5, err_msg="Rank 1, spectrum 0 data")
    else:
        GatherWorkspaces(InputWorkspace=input_ws, AccumulationMethod="Append", ChunkSize=5)


def test_broadcast_then_gather():
    """Test combining broadcast and gather operations"""
    if rank == 0:
        x = np.linspace(0, 5, 26)
        y = np.ones(25) * 10.0
        e = np.ones(25)

        input_ws = CreateWorkspace(DataX=x, DataY=y, DataE=e, NSpec=1, OutputWorkspace="combined_input")

        broadcasted = BroadcastWorkspace(InputWorkspace=input_ws, BroadcasterRank=0, OutputWorkspace="combined_broadcast")
    else:
        broadcasted = BroadcastWorkspace(BroadcasterRank=0, OutputWorkspace="combined_broadcast")

    y_broadcast = broadcasted.readY(0)
    np.testing.assert_array_equal(y_broadcast, np.ones(25) * 10.0, err_msg="Broadcast Y data")

    modified = Scale(InputWorkspace=broadcasted, Factor=rank + 1, OutputWorkspace="combined_modified")

    y_scaled = modified.readY(0)
    expected_scaled = np.ones(25) * 10.0 * (rank + 1)
    np.testing.assert_array_equal(y_scaled, expected_scaled, err_msg=f"Rank {rank} scaled data")

    if rank == 0:
        gathered = GatherWorkspaces(InputWorkspace=modified, AccumulationMethod="Append", OutputWorkspace="combined_gathered")

        assert_equal(gathered.getNumberHistograms(), size, "Number of histograms")

        for r in range(size):
            y_data = gathered.readY(r)
            expected = np.ones(25) * 10.0 * (r + 1)
            np.testing.assert_array_equal(y_data, expected, err_msg=f"Rank {r} gathered data")
    else:
        GatherWorkspaces(InputWorkspace=modified, AccumulationMethod="Append")


def test_gather_error_propagation():
    """Test that errors are properly combined in Add mode"""
    x = np.linspace(0, 5, 11)
    y = np.ones(10) * 5.0
    e = np.ones(10) * 2.0

    input_ws = CreateWorkspace(DataX=x, DataY=y, DataE=e, NSpec=1, OutputWorkspace=f"gather_error_input_rank{rank}")

    if rank == 0:
        output_ws = GatherWorkspaces(InputWorkspace=input_ws, AccumulationMethod="Add", OutputWorkspace="gather_error_output")

        y_vals = output_ws.readY(0)
        expected_y = np.ones(10) * 5.0 * size
        np.testing.assert_array_equal(y_vals, expected_y, err_msg="Summed Y values")

        e_vals = output_ws.readE(0)
        expected_error = np.ones(10) * 2.0 * np.sqrt(size)
        np.testing.assert_array_equal(e_vals, expected_error, err_msg="Error propagation (quadrature)")
    else:
        GatherWorkspaces(InputWorkspace=input_ws, AccumulationMethod="Add")


def test_broadcast_real_workspace():
    """Test broadcasting a real Mantid workspace loaded from file"""

    if rank == 0:
        input_ws = Load(Filename="PG3_9829_event.nxs", OutputWorkspace="real_ws_input")

        original_nspec = input_ws.getNumberHistograms()
        original_nbins = input_ws.blocksize()
        original_y0 = input_ws.readY(0).copy()
        original_x0 = input_ws.readX(0).copy()

        output_ws = BroadcastWorkspace(InputWorkspace=input_ws, BroadcasterRank=0, ChunkSize=0, OutputWorkspace="real_ws_broadcast")
    else:
        output_ws = BroadcastWorkspace(BroadcasterRank=0, ChunkSize=0, OutputWorkspace="real_ws_broadcast")
        original_nspec = None
        original_nbins = None
        original_y0 = None
        original_x0 = None

    original_nspec = comm.bcast(original_nspec, root=0)
    original_nbins = comm.bcast(original_nbins, root=0)
    original_y0 = comm.bcast(original_y0, root=0)
    original_x0 = comm.bcast(original_x0, root=0)

    assert_equal(output_ws.getNumberHistograms(), original_nspec, "Number of histograms")
    assert_equal(output_ws.blocksize(), original_nbins, "Block size")

    np.testing.assert_array_almost_equal(output_ws.readY(0), original_y0, decimal=10, err_msg="Y data")
    np.testing.assert_array_almost_equal(output_ws.readX(0), original_x0, decimal=10, err_msg="X data")

    if rank == 0:
        logger.information(f"Successfully broadcast workspace: {original_nspec} spectra, {original_nbins} bins")


def test_gather_real_workspace():
    """Test gathering real Mantid workspaces loaded from file"""

    input_ws = Load(Filename="PG3_9829_event.nxs", OutputWorkspace=f"real_ws_gather_input_rank{rank}")

    original_nspec = input_ws.getNumberHistograms()
    original_nbins = input_ws.blocksize()

    if rank == 0:
        output_ws = GatherWorkspaces(
            InputWorkspace=input_ws, AccumulationMethod="Append", ChunkSize=0, OutputWorkspace="real_ws_gather_output"
        )

        expected_nspec = original_nspec * size
        assert_equal(output_ws.getNumberHistograms(), expected_nspec, "Total number of histograms")
        assert_equal(output_ws.blocksize(), original_nbins, "Block size")

        original_y0 = input_ws.readY(0)
        for r in range(size):
            spectrum_idx = r * original_nspec
            gathered_y = output_ws.readY(spectrum_idx)
            np.testing.assert_array_almost_equal(gathered_y, original_y0, decimal=10, err_msg=f"Rank {r} first spectrum data")

        logger.information(f"Successfully gathered {size} workspaces: {expected_nspec} total spectra")
    else:
        GatherWorkspaces(InputWorkspace=input_ws, AccumulationMethod="Append", ChunkSize=0)


def test_gather_event_workspace_preserve():
    """Test gathering EventWorkspaces with PreserveEvents=True"""

    input_ws = Load(Filename="PG3_9829_event.nxs", OutputWorkspace=f"event_ws_gather_input_rank{rank}")

    ws_type = input_ws.id()
    if "Event" not in ws_type:
        raise ValueError(f"Expected EventWorkspace, got {ws_type}. Please provide an EventWorkspace file.")

    original_nspec = input_ws.getNumberHistograms()
    original_nevents = input_ws.getNumberEvents()

    if rank == 0:
        output_ws = GatherWorkspaces(
            InputWorkspace=input_ws, AccumulationMethod="Append", PreserveEvents=True, OutputWorkspace="event_ws_gather_output"
        )

        assert "Event" in output_ws.id(), "Output should be an EventWorkspace"

        expected_nspec = original_nspec * size
        assert_equal(output_ws.getNumberHistograms(), expected_nspec, "Total number of histograms")

        expected_nevents = original_nevents * size
        assert_equal(output_ws.getNumberEvents(), expected_nevents, "Total number of events")

        original_events = input_ws.getSpectrum(0)
        gathered_events = output_ws.getSpectrum(0)

        assert_equal(gathered_events.getNumberEvents(), original_events.getNumberEvents(), "Number of events in first spectrum")

        logger.information(f"Successfully gathered {size} EventWorkspaces: {expected_nspec} spectra, {expected_nevents} events")
        logger.information(f"Output is EventWorkspace: {output_ws.id()}")
    else:
        GatherWorkspaces(InputWorkspace=input_ws, AccumulationMethod="Append", PreserveEvents=True)


def main():
    """Run all tests"""
    global passed_tests, failed_tests

    if rank == 0:
        logger.information("\nMPI Algorithm Tests")
        logger.information(f"Running with {size} MPI processes")
        logger.information("=" * 60)

    # Run all tests
    run_test("BroadcastWorkspace - Basic", test_broadcast_basic)
    run_test("BroadcastWorkspace - Chunked", test_broadcast_chunked)
    run_test("BroadcastWorkspace - Non-shared X", test_broadcast_nonshared_x)
    run_test("GatherWorkspaces - Append", test_gather_append)
    run_test("GatherWorkspaces - Add", test_gather_add)
    run_test("GatherWorkspaces - Large Chunk", test_gather_large_chunk)
    run_test("GatherWorkspaces - Error Propagation", test_gather_error_propagation)
    run_test("BroadcastWorkspace then GatherWorkspaces", test_broadcast_then_gather)

    # Tests with real Mantid files
    run_test("BroadcastWorkspace - Real Workspace File", test_broadcast_real_workspace)
    run_test("GatherWorkspaces - Real Workspace File", test_gather_real_workspace)
    run_test("GatherWorkspaces - EventWorkspace PreserveEvents", test_gather_event_workspace_preserve)

    # Synchronize all ranks before logger.informationing summary
    comm.Barrier()

    # Gather failure counts from all ranks
    all_failed_counts = comm.gather(failed_tests, root=0)

    # logger.information summary on rank 0
    if rank == 0:
        logger.information("=" * 60)
        logger.information("TEST SUMMARY")
        logger.information("=" * 60)
        for name, status in test_names:
            logger.information(f"{name}: {status}")

        logger.information("=" * 60)
        logger.information(f"Passed: {passed_tests}/{passed_tests + failed_tests}")
        logger.information(f"Failed: {failed_tests}/{passed_tests + failed_tests}")

        max_failures = max(all_failed_counts)

        if max_failures > 0:
            logger.information("Failures detected across ranks:")
            for r, count in enumerate(all_failed_counts):
                if count > 0:
                    logger.information(f"Rank {r}: {count} failed test(s)")

        logger.information("=" * 60)

        final_exit_code = max_failures

        if final_exit_code == 0:
            logger.information("ALL TESTS PASSED (exit code: 0)")
        else:
            logger.information(f"TEST SUITE FAILED (exit code: {final_exit_code})")
    else:
        final_exit_code = None

    final_exit_code = comm.bcast(final_exit_code, root=0)
    return final_exit_code


if __name__ == "__main__":
    if size < 2:
        if rank == 0:
            logger.information("Error: These tests require at least 2 MPI processes")
            logger.information("Run with: mpiexec -n 4 -l python mpitest.py")
        sys.exit(-1)

    if len(sys.argv) != 2:
        sys.exit(-1)

    data_search_dirs = str(sys.argv[1]).split(",")
    logger.information(str(data_search_dirs))
    temp_config = {"logging.channels.consoleChannel.class": "StdoutChannel"}
    with amend_config(data_dir=data_search_dirs, **temp_config):
        exit_code = main()

    sys.exit(exit_code)
