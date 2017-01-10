"""
Class for commonly used functions across the modules
"""


class Helper(object):
    def __init__(self):
        self._whole_exec_timer = None
        self._timer_running = False
        self._timer_start = None

    def debug_print_memory_usage_linux(self, message=""):
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

    def tomo_print(self, message, priority=1):
        """
        TODO currently the priority parameter is ignored
        Verbosity levels:
        0 -> debug, print everything
        1 -> information, print information about progress
        2 -> print only major progress information, i.e data loaded, recon started, recon finished

        Print only messages that have priority >= config verbosity level

        :param message: Message to be printed
        :param priority: Importance level depending on which messages will be printed
        :return:
        """

        #  should be moved in the configs somewhere
        temp_verbosity = 0
        if priority >= temp_verbosity:
            print(message)

    def tomo_print_timed_start(self, message, priority=1):
        """
        On every second call this will terminate and print the timer
        TODO currently the priority parameter is ignored

        Verbosity levels:
        0 -> debug, print everything
        1 -> information, print information about progress
        2 -> print only major progress information, i.e data loaded, recon started, recon finished

        Print only messages that have priority >= config verbosity level

        :param message: Message to be printed
        :param priority: Importance level depending on which messages will be printed
        :return:
        """

        import time

        #  should be moved in the configs somewhere
        temp_verbosity = 1
        print_string = ""

        if not self._timer_running:
            self._timer_running = True
            self._timer_start = time.time()
            print_string = message

        if priority >= temp_verbosity:
            print(print_string)

    def tomo_print_timed_stop(self, message, priority=1):
        """
        On every second call this will terminate and print the timer. This will append ". " to the string
        TODO currently the priority parameter is ignored

        Verbosity levels:
        0 -> debug, print everything
        1 -> information, print information about progress
        2 -> print only major progress information, i.e data loaded, recon started, recon finished

        Print only messages that have priority >= config verbosity level

        :param message: Message to be printed
        :param priority: Importance level depending on which messages will be printed
        :return:
        """

        import time

        #  should be moved in the configs somewhere
        temp_verbosity = 1
        print_string = ""

        if self._timer_running:
            self._timer_running = False
            timer_string = str(time.time() - self._timer_start)
            print_string = message + " Elapsed time: " + timer_string + " sec"

        if priority >= temp_verbosity:
            print(print_string)

    def tomo_total_timer(self, message="Total execution time was "):
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
