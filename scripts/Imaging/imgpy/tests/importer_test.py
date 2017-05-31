from __future__ import (absolute_import, division, print_function)
import unittest


class ImporterTest(unittest.TestCase):
    def __init__(self, *args, **kwargs):
        super(ImporterTest, self).__init__(*args, **kwargs)

        # force silent outputs
        from configs.recon_config import ReconstructionConfig
        self.config = ReconstructionConfig.empty_init()
        self.config.func.verbosity = 0

    # that's the only supported tool right now, Astra is used through TomoPy
    def test_tomopy(self):
        from tools.tomopy_tool import TomoPyTool
        from tools import importer
        self.config.func.tool = 'tomopy'
        tool = importer.timed_import(self.config)
        assert isinstance(tool, TomoPyTool)

        tool = importer.do_importing('tomopy')
        assert isinstance(tool, TomoPyTool)

    def test_tomopy_astra(self):
        from tools.astra_tool import AstraTool
        from tools import importer
        self.config.func.tool = 'astra'
        self.config.func.algorithm = 'fbp'
        tool = importer.timed_import(self.config)
        assert isinstance(tool, AstraTool)

        tool = importer.do_importing('astra')
        assert isinstance(tool, AstraTool)

    def test_tomopy_astra_not_supported_alg(self):
        # not supported algorithm
        from tools import importer
        self.config.func.tool = 'astra'
        self.config.func.algorithm = 'gridrec'
        self.assertRaises(ValueError, importer.timed_import, self.config)

    def test_not_supported_tool(self):
        # not supported tool
        from tools import importer
        self.config.func.tool = 'boop'
        self.assertRaises(ValueError, importer.timed_import, self.config)

        # invalid tool parameter
        self.config.func.tool = 42
        self.assertRaises(TypeError, importer.timed_import, self.config)


if __name__ == '__main__':
    unittest.main()
