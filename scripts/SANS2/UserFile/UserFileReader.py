from collections import Sequence
from SANS2.Common.SANSFileInformation import find_full_file_path
from SANS2.UserFile.UserFileParser import UserFileParser
from SANS2.UserFile.UserFileCommon import *


class UserFileReader(object):
    def __init__(self, user_file):
        super(UserFileReader, self).__init__()
        self._user_file = find_full_file_path(user_file)

    @staticmethod
    def _add_to_output(output, parsed):
        # If the parsed values already exist in the output dict, then we extend the output
        # else we just add it to the output dict.
        for key, value in parsed.items():
            if key in output:
                output[key].append(value)
            else:
                output[key] = [value]

    @staticmethod
    def _check_output(output):
        pass
        # ADD SEMANTIC CHECKS HERE!!!!!!
        # This is the place to check if there are logical errors in the user file and to inform the user that
        # something is not quite right

    def read_user_file(self):
        # Read in all elements
        parser = UserFileParser()

        output = {}
        with open(self._user_file) as f:
            for line in f:
                parsed = parser.parse_line(line)
                UserFileReader._add_to_output(output, parsed)

        # Check elements which require checking
        UserFileReader._check_output(output)

        # Provide the read elements
        return output
