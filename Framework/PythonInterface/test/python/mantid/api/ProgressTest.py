import unittest
from mantid.api import Progress,PythonAlgorithm
from mantid.kernel import ProgressBase

class ProgressTest(unittest.TestCase):

    def test_class_inherits_ProgressBase(self):
        self.assertTrue(issubclass(Progress,ProgressBase))

    def test_object_can_be_constructed_with_PythonAlgorithm(self):
        class ProgressConstructTestAlgorithm(PythonAlgorithm):

            def PyInit(self):
                pass
            def PyExec(self):
                prog_reporter = Progress(self,0.0,1.0,100)
                if not isinstance(prog_reporter, Progress):
                    raise RuntimeError("Object constructed but it is not of the correct type!")

        # Test
        test_alg = ProgressConstructTestAlgorithm()
        test_alg.initialize()
        test_alg.execute()

    def test_object_can_be_successfully_report_with_PythonAlgorithm(self):
        class ProgressReportTestAlgorithm(PythonAlgorithm):

            def PyInit(self):
                pass
            def PyExec(self):
                prog_reporter = Progress(self,0.0,1.0,100)
                prog_reporter.report()
        # Test
        test_alg = ProgressReportTestAlgorithm()
        test_alg.initialize()
        test_alg.execute()


if __name__ == '__main__':
    unittest.main()
