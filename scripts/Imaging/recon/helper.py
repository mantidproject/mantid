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
    """
    Helper class for functions used accross the 
    """

    def __init__(self, config=None):
        """
        Initialises the Helper class.
        If a ReconstructionConfig is provided, it will
        :param config: The ReconstructionConfig with which this class will be initialised
        """

        # If a config is provided make sure it's the proper class
        if config is not None and not isinstance(config, ReconstructionConfig):
            raise ValueError("The provided config is not of the correct type ReconstructionConfig!\n\t"
                             "    If no config is available (e.g. running a standalone filter) use an empty constructor.")
            # so much space so it aligns in the terminal

        self._whole_exec_timer = None
        self._timer_running = False
        self._timer_start = None
        self._timer_print_prefix = " ---"

        self._verbosity = 2 if config is None else config.func.verbosity
        self._no_crash_on_failed_import = False if config is None else config.func.no_crash_on_failed_import

        self._cache_last_memory = None

        self._progress_bar = None

        if config is None:
            self.tomo_print_warning("Helper class initialised without config!")

    def get_verbosity(self):
        return self._verbosity

    @staticmethod
    def check_config_integrity(config):
        if not config or not isinstance(config, ReconstructionConfig):
            raise ValueError(
                "Cannot run a reconstruction without a valid configuration")

        if not config.func.input_path:
            raise ValueError(
                "Cannot run a reconstruction without setting the input path")

        if not config.func.output_path:
            raise ValueError(
                "Cannot run a reconstruction without setting the output path")

    @staticmethod
    def empty_init():
        return Helper()

    @staticmethod
    def check_data_stack(data):
        import numpy

        if not isinstance(data, numpy.ndarray):
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
            # Windows doesn't seem to have resource package, so this will
            # silently fail
            import resource as res
            print(" >> Memory usage",
                  res.getrusage(res.RUSAGE_SELF).ru_maxrss, "KB, ",
                  int(res.getrusage(res.RUSAGE_SELF).ru_maxrss) /
                  1024, "MB", message)
        except ImportError:
            res = None
            pass

    def get_memory_usage_linux(self):
        try:
            # Windows doesn't seem to have resource package, so this will
            # silently fail
            import resource as res

            memory_in_kbs = int(res.getrusage(res.RUSAGE_SELF).ru_maxrss)

            memory_in_mbs = int(res.getrusage(
                res.RUSAGE_SELF).ru_maxrss) / 1024

            # handle caching
            memory_string = " {0} KB, {1} MB".format(
                memory_in_kbs, memory_in_mbs)

            if self._cache_last_memory is None:
                self._cache_last_memory = memory_in_kbs
            else:
                # get memory difference in Megabytes
                delta_memory = (memory_in_kbs - self._cache_last_memory) / 1024

                # remove cached memory
                self._cache_last_memory = None
                memory_string += ". Memory change: {0} MB".format(delta_memory)

        except ImportError:
            memory_string = " <not available on Windows> "

        return memory_string

    def tomo_print_same_line(self, message, verbosity=2):
        """
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
            print(message, end='')

    def tomo_print(self, message, verbosity=2):
        """
        Verbosity levels:
        0 -> debug, print everything
        1 -> information, print information about progress
        2 -> print only major progress information, i.e data loaded, recon started, recon finished

        Print only messages that have priority >= config verbosity level

        :param message: Message to be printed
        :param verbosity: Default 2, messages with existing levels:

            0 - Silent, no output at all (not recommended)
            1 - Low verbosity, will output each step that is being performed and important warnings/errors
            2 - Normal verbosity, will output each step and execution time
            3 - High verbosity, will output the step name, execution time and memory usage before and after each step.
                THE MEMORY USAGE DOES NOT WORK ON WINDOWS.
                This will probably use more resources.
        """

        # will be printed if the message verbosity is lower or equal
        # i.e. level 1,2,3 messages will not be printed on level 0 verbosity
        if verbosity <= self._verbosity:
            print(message)

    def tomo_print_note(self, message, verbosity=2):
        self.tomo_print(" > Note: " + message, verbosity)

    def tomo_print_warning(self, message, verbosity=2):
        self.tomo_print(" >> WARNING: " + message, verbosity)

    def tomo_print_error(self, message, verbosity=1):
        self.tomo_print(" >>> ERROR: " + message, verbosity)

    def pstart(self, message, verbosity=2):
        """
        Print the message and start the execution timer.

        This will conform to the same verbosity restrictions as tomo_print(...).

        :param message: Message to be printed
        :param verbosity: See tomo_print(...)
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
            print(self._timer_print_prefix, print_string)

    def pstop(self, message, verbosity=2):
        """
        Print the message and stop the execution timer.

        This will conform to the same verbosity restrictions as tomo_print(...).

        :param message: Message to be printed
        :param verbosity: See tomo_print(...)
        """

        import time

        print_string = ""

        if self._verbosity >= 2 and self._timer_running:
            self._timer_running = False
            timer_string = str(time.time() - self._timer_start)
            print_string += (message + " Elapsed time: " +
                             timer_string + " sec.")

        if self._verbosity >= 3:
            print_string += " Memory usage after execution: " + self.get_memory_usage_linux()

        # will be printed if the message verbosity is lower or equal
        # i.e. level 1,2,3 messages will not be printed on level 0 verbosity
        if verbosity <= self._verbosity:
            print(self._timer_print_prefix, print_string)

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

    def handle_exception(self, exception):
        """
        TODO The plan for this function is to apply
        the --no-crash-on-failed-import (rename to crash on exception?) throughout
        the scripts, so that they use a common function

        Currently only re-raises the exception so that it is not lost silently!
        """
        raise exception

    def prog_init(self, total, desc="Progress", ascii=False, unit='images'):
        """
        Initialises and returns the progress bar if the tqdm library is available.

        Otherwise does nothing, and Helper's progress update function will also do nothing.
        """
        try:
            from tqdm import tqdm
            self._progress_bar = tqdm(
                total=total, desc=desc, ascii=ascii, unit=unit)
        except ImportError:
            self._progress_bar = None
            self.tomo_print_note("tqdm library not found, progress bars will not appear. You could install"
                                 " locally with pip install tqdm")

    def prog_update(self, update):
        if self._progress_bar is not None:
            self._progress_bar.update(update)
