# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import concurrent.futures
import sys

from mantid.simpleapi import Load
from systemtesting import MantidSystemTest, linux_distro_description


def load_nexus_in_multiple_threads(filename, nthreads):
    """Attempt to load the the given filename from multiple threads at once
    """
    results = [None for i in range(nthreads)]
    with concurrent.futures.ThreadPoolExecutor(max_workers=nthreads) as executor:
        jobs = [executor.submit(load_nexus, filename, index) for index in range(nthreads)]
        for index, future in enumerate(concurrent.futures.as_completed(jobs)):
            try:
                future.result()
            except Exception as exc:
                results[index] = str(exc)
            else:
                results[index] = None

    raise_error_if_failed(results)


def load_nexus(filename: str, index: int) -> None:
    """Callable for Thread object. Performs the Load call
    :param filename: NeXus filename to load
    :param index: Index of thread assigned to perform load
    """
    Load(filename, OutputWorkspace=f'w{index}')


def raise_error_if_failed(results) -> None:
    """Raises a RuntimeError if any failure occurred"""
    if not all(map(lambda x: x is None, results)):
        messages = ["It was not possible to load a NeXus file with multiple threads. Errors:"]
        for index, msg in enumerate(filter(lambda x: x is not None, results)):
            messages.append(f'Thread {index} raised an error: {msg}')

        raise RuntimeError('\n'.join(messages))


class MultiThreadedLoadNeXusTest(MantidSystemTest):
    """Verify that a NeXus file can be loaded
       from multiple threads.
       HDF5 can be built without the threadsafe option
       which causes problems for the current how the
       framework accesses NeXus files.
    """
    NTHREADS = 2

    def skipTests(self):
        """HDF5 is currently not built in threadsafe mode on RHEL or macOS"""
        # Ideally this would be a capability check but that's very difficult as
        # the RHEL library doesn't have the H5is_library_threadsafe function
        if sys.platform == 'linux':
            distro = linux_distro_description().lower()
            is_redhat_like = [name in distro for name in ('red hat', 'centos', 'fedora')]
            return any(is_redhat_like)
        elif sys.platform == 'darwin':
            return True
        else:
            return False

    def runTest(self):
        """Spin up multiple threads and simply
        check that we don't crash"""
        # "Raw" data NeXus
        load_nexus_in_multiple_threads(filename='INTER00013463.nxs', nthreads=self.NTHREADS)
        # Mantid processed file
        load_nexus_in_multiple_threads(filename='MARIReductionAutoEi.nxs', nthreads=self.NTHREADS)
