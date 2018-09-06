from mantid.kernel import ConfigService
import os
import shutil
import hashlib
import datetime

class RetrieveRecoveryFiles(object):
    def __init__(self):
        pass

    @staticmethod
    def _get_properties_directory():
        return ConfigService.getUserPropertiesDir()

    @staticmethod
    def get_recovery_files_path():
        recovery_files_path = ''
        properties_directory = RetrieveRecoveryFiles._get_properties_directory()
        if 'recovery' not in os.listdir(properties_directory):
            return recovery_files_path

        recovery_dir_contents = os.listdir(properties_directory + 'recovery')
        if not recovery_dir_contents:
            return recovery_files_path

        recovery_files_path = properties_directory + 'recovery'
        return recovery_files_path

    @staticmethod
    def zip_recovery_directory():
        path = RetrieveRecoveryFiles.get_recovery_files_path()
        directory = RetrieveRecoveryFiles._get_properties_directory()
        hash_value = hashlib.md5(str.encode(directory + str(datetime.datetime.now())))
        zip_file = directory + hash_value.hexdigest()
        if path:
            shutil.make_archive(zip_file, 'zip', path)
            return zip_file, hash_value.hexdigest()
        return ''

