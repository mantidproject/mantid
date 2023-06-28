from systemtesting import MantidSystemTest


# Test that fails to test that failing system tests will end a build
class FailingSystemTest(MantidSystemTest):
    def runTest(self):
        raise RuntimeError("The fake test failed!")
