from mantid.kernel import ConfigService
import os
import shutil
import hashlib
import datetime


def get_properties_directory():
    return ConfigService.getAppDataDirectory()


def get_recovery_files_path():
    properties_directory = get_properties_directory()
    if 'recovery' not in os.listdir(properties_directory):
        return None

    recovery_files_path = os.path.join(properties_directory, 'recovery')
    if len(os.listdir(recovery_files_path)) > 0:
        return recovery_files_path
    else:
        return None


def zip_recovery_directory():
    path = get_recovery_files_path()
    if path is None:
        return "", ""
    directory = get_properties_directory()
    hash_value = hashlib.md5(str.encode(directory + str(datetime.datetime.now())))
    base_name = os.path.join(directory, hash_value.hexdigest())
    zip_file = shutil.make_archive(base_name, 'zip', path)
    return zip_file, hash_value.hexdigest()
