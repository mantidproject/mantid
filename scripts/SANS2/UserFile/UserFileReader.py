from SANS2.Common.SANSFileInformation import find_full_file_path
from SANS2.UserFile.UserFileParser import UserFileParser


class UserFileReader(object):
    def __init__(self, user_file):
        super(UserFileReader, self).__init__()
        self._user_file = find_full_file_path(user_file)

    @staticmethod
    def _add_to_output(output, parsed):
        # If the parsed values already exist in the output dict, then we extend the output
        # else we just add it to the output dict. We have to be careful if we are dealing with a sequence. The scenarios
        # are:
        # 1. Exists and is standard value => add it to the existing list
        # 2. Exists and is a sequence type => extend the existing list
        # 3. Does not exist and is a standard value => create a list with that value and add it
        # 4. Does not exist and is a sequence type => add the list itself
        for key, value in parsed.items():
            is_list = isinstance(value, list)
            is_key_in_output = key in output
            if is_key_in_output and is_list:
                print value
                output[key].extend(value)
            elif is_key_in_output and not is_list:
                output[key].append(value)
            elif not is_key_in_output and is_list:
                output[key] = value
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
