#pylint: disable=no-init
import stresstesting
import sys
import os

class UserAlgorithmsBuild(stresstesting.MantidStressTest):

    build_success = False

    def skipTests(self):
        " We skip this test if the system is not Windows."
        if sys.platform.startswith('win'):
            return False
        else:
            return True

    def runTest(self):
        """
            System test for testing that the UserAlgorithm build script works
        """
        # Run the build
        import subprocess
        retcode = subprocess.call(["C:\\MantidInstall\\UserAlgorithms\\build.bat","--quiet"])
        if retcode == 0:
            self.build_success = True
        else:
            self.build_success = False

    def cleanup(self):
        # Remove build files as they will be loaded by the next
        # process that runs this test and it then can't remove them!
        install_dir = r'C:\MantidInstall\plugins'
        lib_name = 'UserAlgorithms'
        exts = ['.dll', '.exp', '.lib']
        for ext in exts:
            try:
                os.remove(os.path.join(install_dir, lib_name + ext))
            except OSError:
                pass

    def validate(self):
        return self.build_success
