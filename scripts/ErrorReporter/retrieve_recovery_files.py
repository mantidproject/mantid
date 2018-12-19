from mantid.kernel import ConfigService
import os
import shutil
import hashlib
import datetime


def get_properties_directory():
    return ConfigService.getAppDataDirectory()


def get_recovery_files_path():
    recovery_files_path = ''
    properties_directory = get_properties_directory()
    if 'recovery' not in os.listdir(properties_directory):
        return recovery_files_path

    recovery_dir_contents = os.listdir(properties_directory + 'recovery')
    if not recovery_dir_contents:
        return recovery_files_path

    recovery_files_path = properties_directory + 'recovery'
    return recovery_files_path


def zip_recovery_directory():
    path = get_recovery_files_path()
    directory = get_properties_directory()
    hash_value = hashlib.md5(str.encode(directory + str(datetime.datetime.now())))
    zip_file = os.path.join(directory, hash_value.hexdigest())
    if path:
        shutil.make_archive(zip_file, 'zip', path)
        return zip_file, hash_value.hexdigest()
    return ''


def remove_recovery_file(file):
    directory = get_properties_directory()
    zip_file = os.path.join(directory, file)
    os.remove(zip_file + '.zip')
