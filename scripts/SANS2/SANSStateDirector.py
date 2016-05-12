import os
import re
from mantid.api import FileFinder

from UserFileParser import UserFileParser

# ----------------------------------------------------
# Free Functions
# ----------------------------------------------------
def is_valid_user_file_extension(user_file):
    """
    Check if the file name has a valid extension for user files.
    This can be either txt or something similar to .09A
    """
    allowed_values = ['.TXT']
    # We need to allow for old user file formats. They started with a number.
    # But there doesn't seem to be a general format. As a very basic check
    # we make sure that the ending starts with a number
    pattern = r'^\.[0-9]+'
    _filename, file_extension = os.path.splitext(user_file)
    file_extension = file_extension.upper()
    is_allowed = False
    if file_extension in allowed_values or re.match(pattern, file_extension):
        is_allowed = True
    return is_allowed


def get_user_full_user_file_path(user_file_name):
    if not os.path.isfile(user_file_name):
        user_file = FileFinder.getFullPath(user_file_name)
        if not os.path.isfile(user_file):
            raise ValueError("Cannot read User File. File path {} does not exist"
                             " or is not in the user path.".format(user_file_name))
        user_file_name = user_file
    return user_file_name


class SANSStateDirectorUserFile(object):
    """
    This director is responsible for providing user settings from a UserFile (via a Parser) to a
    SANSStateBuilder which will build a state. It essentially builds a SANSStateComplete object from
    a user file. Note that this SANSState object will not be valid, since it will not contain information
    about the data files which are to be used.
    """
    def __init__(self, user_file_name):
        super(SANSStateDirectorUserFile, self).__init__()

        # Check that the format is valid, ie txt or 099AA else raise
        if not is_valid_user_file_extension(user_file_name):
            raise ValueError("SANSStateDirectorUserFile: The user file does not seem to be of the correct file type.")

        # Get the full file path if it exists
        self._user_file_name = get_user_full_user_file_path(user_file_name)

        # Parsed user file entries
        self._entries = {}

    def parse_user_file(self):
        parser = UserFileParser()
        self._entries.clear()

        with open(self._user_file_name, "r") as user_file:
            for line in user_file:
                information = parser.parse_line(line)
                self._entries.update(information)
