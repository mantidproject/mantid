from __future__ import (absolute_import, division, print_function)
import sys
"""
Class for commonly used functions across the modules

verbosity: Default 2, existing levels:
    0 - Silent, no output at all (not recommended)
    1 - Low verbosity, will output each step that is being performed
    2 - Normal verbosity, will output each step and execution time
    3 - High verbosity, will output the step name, execution time and memory usage before and after each step
"""
from configs.recon_config import ReconstructionConfig


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
        self.check_config_class(config)

        self._whole_exec_timer = None
        self._timer_running = False
        self._timer_start = None
        self._timer_print_prefix = " ---"

        self._verbosity = 2 if config is None else config.func.verbosity

        self._cache_last_memory = None

        self._note_str = " > Note: "
        self._warning_str = " >> WARNING: "
        self._error_str = " >>> ERROR: "

        # for timer
        self._progress_bar = None

        # sneaky copy of the config
        self.config = config

        self._readme = None

    def get_verbosity(self):
        return self._verbosity

    @staticmethod
    def check_config_class(config):
        assert isinstance(config, ReconstructionConfig), "The provided config is invalid and cannot be used. " \
                                                         "You can create a Helper instantiation with an empty " \
                                                         "ReconstructionConfig using Helper.empty_init()"

    def check_config_integrity(self, config):
        self.check_config_class(config)

        if not config.func.output_path:
            self.tomo_print_warning(
                "No output path specified, no output will be produced!")

    @staticmethod
    def empty_init():
        return Helper(ReconstructionConfig.empty_init())

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
            print(" >> Memory usage", res.getrusage(res.RUSAGE_SELF).ru_maxrss,
                  "KB, ", int(res.getrusage(res.RUSAGE_SELF).ru_maxrss) / 1024,
                  "MB", message)
        except ImportError:
            res = None
            pass

    def progress_available(self):
        try:
            from tqdm import tqdm
        except ImportError:
            self.tomo_print_note(
                "Progress bar library TQDM not available. "
                "To install locally please use pip install tqdm. "
                "Falling back to ASCII progress bar.")

    def run_import_checks(self):
        """
        Run the import checks to notify the user which features are available in the execution.
        """
        from parallel import utility as pu
        self.progress_available()

        if not pu.multiprocessing_available():
            self.tomo_print_note("Multiprocessing not available.")
        else:
            self.tomo_print_note(
                "Running process on {0} cores.".format(self.config.func.cores))

    def get_memory_usage_linux(self):
        try:
            # Windows doesn't seem to have resource package, so this will
            # silently fail
            import resource as res

            memory_in_kbs = int(res.getrusage(res.RUSAGE_SELF).ru_maxrss)

            memory_in_mbs = int(
                res.getrusage(res.RUSAGE_SELF).ru_maxrss) / 1024

            # handle caching
            memory_string = " {0} KB, {1} MB".format(memory_in_kbs,
                                                     memory_in_mbs)

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
        :param message: Message to be printed
        :param verbosity: See tomo_print(...)
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
        if self._readme is not None:
            self._readme.append(message)

        if verbosity <= self._verbosity:
            print(message)

    def tomo_print_note(self, message, verbosity=2):
        self.tomo_print(self._note_str + message, verbosity)

    def tomo_print_warning(self, message, verbosity=2):
        self.tomo_print(self._warning_str + message, verbosity)

    def tomo_print_error(self, message, verbosity=1):
        self.tomo_print(self._error_str + message, verbosity)

    def pstart(self, message, verbosity=2):
        """
        Print the message and start the execution timer.

        :param message: Message to be printed
        :param verbosity: See tomo_print(...)
        """

        if not self._timer_running:
            self._timer_running = True

        import time
        print_string = str(message)
        # will be printed on levels 2 and 3
        if self._verbosity >= 2:
            self._timer_start = time.time()

        # will be printed on level 3 only
        if self._verbosity >= 3:
            print_string += " Memory usage before execution: " + self.get_memory_usage_linux(
            )

        self.tomo_print(self._timer_print_prefix + print_string, verbosity)

    def pstop(self, message, verbosity=2):
        """
        Print the message and stop the execution timer.

        :param message: Message to be printed
        :param verbosity: See tomo_print(...)
        """

        if not self._timer_running:
            raise ValueError("helper.pstart(...) was not called previously!")

        import time
        print_string = ""
        if self._verbosity >= 2:
            self._timer_running = False
            timer_string = str(time.time() - self._timer_start)
            print_string += (
                str(message) + " Elapsed time: " + timer_string + " sec.")

        if self._verbosity >= 3:
            print_string += " Memory usage after execution: " + self.get_memory_usage_linux(
            )

        self.tomo_print(self._timer_print_prefix + print_string, verbosity)

    def total_execution_timer(self, message="Total execution time was "):
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
            message += self._whole_exec_timer + " sec"
            if self._readme is not None:
                self._readme.append(message)
            print(message)

    def prog_init(self, total, desc="Progress", ascii=False, unit='images'):
        """
        Initialises and returns the progress bar if the tqdm library is available.

        Otherwise does nothing, and Helper's progress update and close function will also do nothing.

        :param total: the total number of iterations of the progress bar
        :param desc: the label in front of the progress bar
        :param ascii: to use ascii # (True) or utf-8 blocks (False)
        :param unit: the unit for loading. Default is 'images'
        """
        if self._verbosity > 0:
            try:
                from tqdm import tqdm
                if self._progress_bar is not None:
                    raise ValueError(
                        "Timer was not closed previously. Please do prog_close()!"
                    )
                self._progress_bar = tqdm(
                    total=total, desc=desc, ascii=ascii, unit=unit)
            except ImportError:
                try:
                    from recon.custom_timer import CustomTimer
                    self._progress_bar = CustomTimer(total, desc)
                except ImportError:
                    self._progress_bar = None

    def prog_update(self, value=1):
        """
        This function will print a simple ascii bar if tqdm is not present.
        """
        if self._progress_bar is not None:
            self._progress_bar.update(value)

    def prog_close(self):
        """
        This function will do nothing if the tqdm library is not present.
        """

        if self._progress_bar is not None:
            self._progress_bar.close()

        self._progress_bar = None

    def set_readme(self, readme):
        self._readme = readme

    def save_debug(self, sample, config, flat, dark, path_append, *args):
        return
        # if any of the arguments are none
        for a in args:
            if a is None or a is False:
                return

        from imgdata.saver import Saver

        saver = Saver(config)
        saver._img_format = 'fits'  # force fits files
        saver.save_single_image(sample, subdir=path_append, name='sample')

        saver._data_as_stack = True  # force data as stack to save out single images
        if flat is not None:
            saver.save_single_image(flat, subdir=path_append, name='flat')

        if dark is not None:
            saver.save_single_image(dark, subdir=path_append, name='dark')
