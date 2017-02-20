from __future__ import (absolute_import, division, print_function)

# This way ALL separate instances of a Readme will write to the same output!
total_string = ""

class Readme(object):
    def __init__(self, config, saver, h):
        import os
        h.check_config_class(config)
        self._readme_file_name = config.func.readme_file_name
        self._output_path = None
        self._readme_fullpath = None
        self._total_string = ""
        self._output_path = saver.get_output_path()
        self._readme_fullpath = None
        if self._output_path is not None:
            self._readme_fullpath = os.path.join(
                self._output_path, self._readme_file_name)

    def append(self, string):
        """
        Append the string to the full string. This appends a new line character.

        :param string: string to be appended
        """
        global total_string
        total_string += string + '\n'

    def begin(self, cmd_line, config):
        """
        To write configuration, settings, etc. early on. As early as possible, before any failure
        can happen.

        :param cmd_line :: command line originally used to run this reconstruction, when running
        from the command line
        :param config :: the full reconstruction configuration so it can be dumped into the file

        Returns :: time now (begin of run) in number of seconds since epoch (time() time)
        """

        if self._readme_fullpath is None:
            return

        import time
        tstart = time.time()

        # generate file with dos/windows line end for windows users convenience
        with open(self._readme_fullpath, 'w') as oreadme:
            file_hdr = (
                'Tomographic reconstruction. Summary of inputs, settings and outputs.\n'
                'Time now (run begin): ' + time.ctime(tstart) + '\n')
            oreadme.write(file_hdr)

            alg_hdr = ("\n"
                       "--------------------------\n"
                       "Tool/Algorithm\n"
                       "--------------------------\n")
            oreadme.write(alg_hdr)
            oreadme.write(str(config.func))
            oreadme.write("\n")

            preproc_hdr = ("\n"
                           "--------------------------\n"
                           "Pre-processing parameters\n"
                           "--------------------------\n")
            oreadme.write(preproc_hdr)
            oreadme.write(str(config.pre))
            oreadme.write("\n")

            postproc_hdr = ("\n"
                            "--------------------------\n"
                            "Post-processing parameters\n"
                            "--------------------------\n")
            oreadme.write(postproc_hdr)
            oreadme.write(str(config.post))
            oreadme.write("\n")

            cmd_hdr = ("\n"
                       "--------------------------\n"
                       "Command line\n"
                       "--------------------------\n")
            oreadme.write(cmd_hdr)
            oreadme.write(cmd_line)
            oreadme.write("\n")

    def end(self):
        """
        Write last part of report in the output readme/report file. This should be used whenever a
        reconstruction runs correctly.

        :param data_stages :: tuple with data in three stages (raw, pre-processed, reconstructed)
        :param tstart :: time at the beginning of the job/reconstruction, when the first part of the
        readme file was written
        :param t_recon_elapsed :: reconstruction time
        """

        # append to a readme/report that should have been pre-filled with the
        # initial configuration
        if self._readme_fullpath is None:
            return

        with open(self._readme_fullpath, 'a') as oreadme:
            oreadme.write(self._total_string)
