from __future__ import (absolute_import, division, print_function)

"""
Class for commonly used functions across the modules

verbosity: Default 2, existing levels:
    0 - Silent, no output at all (not recommended)
    1 - Low verbosity, will output each step that is being performed
    2 - Normal verbosity, will output each step and execution time
    3 - High verbosity, will output the step name, execution time and memory usage before and after each step
"""
from recon.configs.recon_config import ReconstructionConfig


class Helper(object):
    def __init__(self, config=None):
        """
        :param config:
        """
        self._whole_exec_timer = None
        self._timer_running = False
        self._timer_start = None
        self._verbosity = 2 if config is None else config.verbosity

    @staticmethod
    def check_config_integrity(cfg):
        if not cfg or not isinstance(cfg, ReconstructionConfig):
            raise ValueError(
                "Cannot run a reconstruction without a valid configuration")

        if not cfg.preproc_cfg.input_dir:
            raise ValueError(
                "Cannot run a reconstruction without setting the input path")

        if not cfg.postproc_cfg.output_dir:
            raise ValueError(
                "Cannot run a reconstruction without setting the output path")

    @staticmethod
    def check_data_stack(data):
        import numpy as np

        if not isinstance(data, np.ndarray):
            raise ValueError(
                "Invalid stack of images data. It is not a numpy array: {0}".
                format(data))

        if 3 != len(data.shape):
            raise ValueError(
                "Invalid stack of images data. It does not have 3 dimensions. Shape: {0}".
                format(data.shape))

    @staticmethod
    def debug_print_memory_usage_linux(message=""):
        try:
            # Windows doesn't seem to have resource package, so this will silently fail
            import resource as res
            print(" >> Memory usage",
                  res.getrusage(res.RUSAGE_SELF).ru_maxrss, "KB, ",
                  int(res.getrusage(res.RUSAGE_SELF).ru_maxrss) /
                  1024, "MB", message)
        except ImportError:
            res = None
            pass

    @staticmethod
    def get_memory_usage_linux():
        try:
            # Windows doesn't seem to have resource package, so this will silently fail
            import resource as res
            memory_string = " {0} KB, {1} MB".format(
                  res.getrusage(res.RUSAGE_SELF).ru_maxrss, int(res.getrusage(res.RUSAGE_SELF).ru_maxrss) / 1024)
        except ImportError:
            res = None
            memory_string = " <not available on Windows>"

        return memory_string

    def tomo_print(self, message, verbosity=2):
        """
        TODO currently the priority parameter is ignored
        Verbosity levels:
        0 -> debug, print everything
        1 -> information, print information about progress
        2 -> print only major progress information, i.e data loaded, recon started, recon finished

        Print only messages that have priority >= config verbosity level

        :param message: Message to be printed
        :param verbosity: Default 2, messages with existing levels:

            0 - Silent, no output at all (not recommended)
            1 - Low verbosity, will output each step that is being performed
            2 - Normal verbosity, will output each step and execution time
            3 - High verbosity, will output the step name, execution time and memory usage before and after each step
        :return:
        """

        # will be printed if the message verbosity is lower or equal
        # i.e. level 1,2,3 messages will not be printed on level 0 verbosity
        if verbosity <= self._verbosity:
            print(message)

    def pstart(self, message, verbosity=2):
        """
        On every second call this will terminate and print the timer
        TODO currently the priority parameter is ignored

        :param message: Message to be printed
        :param verbosity: See tomo_print(...) for
        :return:
        """

        import time

        print_string = message

        # will be printed on levels 2 and 3
        if self._verbosity >= 2 and not self._timer_running:
            self._timer_running = True
            self._timer_start = time.time()

        # will be printed on level 3 only
        if self._verbosity >= 3:
            print_string += " Memory usage before execution: " + self.get_memory_usage_linux()

        # will be printed if the message verbosity is lower or equal
        # i.e. level 1,2,3 messages will not be printed on level 0 verbosity
        if verbosity <= self._verbosity:
            print(print_string)

    def pstop(self, message, verbosity=2):
        """
        On every second call this will terminate and print the timer. This will append ". " to the string
        TODO currently the priority parameter is ignored

        Verbosity levels:
        0 -> debug, print everything
        1 -> information, print information about progress
        2 -> print only major progress information, i.e data loaded, recon started, recon finished

        Print only messages that have priority >= config verbosity level

        :param message: Message to be printed
        :param verbosity: Importance level depending on which messages will be printed
        :return:
        """

        import time

        print_string = None

        if self._verbosity >= 2 and self._timer_running:
            self._timer_running = False
            timer_string = str(time.time() - self._timer_start)
            print_string = message + " Elapsed time: " + timer_string + " sec."

        if self._verbosity >= 3:
            print_string += " Memory usage after execution: " + self.get_memory_usage_linux()

        # will be printed if the message verbosity is lower or equal
        # i.e. level 1,2,3 messages will not be printed on level 0 verbosity
        if verbosity <= self._verbosity:
            print(print_string)

    def total_reconstruction_timer(self, message="Total execution time was "):
        """
        This will ONLY be used to time the WHOLE execution time.
        The first call to this will be in tomo_reconstruct.py and it will start it.abs
        The last call will be at the end of find_center or do_recon.
        """
        import time

        if not self._whole_exec_timer:
            # change type from bool to timer
            self._whole_exec_timer = time.time()
        else:
            # change from timer to string
            self._whole_exec_timer = str(time.time() - self._whole_exec_timer)
            print(message + self._whole_exec_timer + " sec")
