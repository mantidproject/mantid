from __future__ import (absolute_import, division, print_function)
import unittest


class ImporterTest(unittest.TestCase):
    def __init__(self, *args, **kwargs):
        super(ImporterTest, self).__init__(*args, **kwargs)

        # force silent outputs
        from configs.recon_config import ReconstructionConfig
        r = ReconstructionConfig.empty_init()
        r.func.verbosity = 0
        from helper import Helper

        self.h = Helper(r)

    # that's the only supported tool right now, Astra is used through TomoPy
    def test_tomopy(self):
        from recon.tools.tomopy_tool import TomoPyTool
        from recon.tools import importer
        self.h.config.func.tool = 'tomopy'
        tool = importer.timed_import(self.h.config, self.h)
        assert isinstance(tool, TomoPyTool)

        tool = importer.do_importing('tomopy')
        assert isinstance(tool, TomoPyTool)

    def test_tomopy_astra(self):
        from recon.tools.astra_tool import AstraTool
        from recon.tools import importer
        self.h.config.func.tool = 'astra'
        self.h.config.func.algorithm = 'fbp'
        tool = importer.timed_import(self.h.config, self.h)
        assert isinstance(tool, AstraTool)

        tool = importer.do_importing('astra')
        assert isinstance(tool, AstraTool)

    def test_tomopy_astra_not_supported_alg(self):
        # not supported algorithm
        from recon.tools import importer
        self.h.config.func.tool = 'astra'
        self.h.config.func.algorithm = 'gridrec'
        self.assertRaises(ValueError, importer.timed_import, self.h.config, self.h)

    def test_not_supported_tool(self):
        # not supported tool
        from recon.tools import importer
        self.h.config.func.tool = 'boop'
        self.assertRaises(ValueError, importer.timed_import, self.h.config, self.h)

        # invalid tool parameter
        self.h.config.func.tool = 42
        self.assertRaises(TypeError, importer.timed_import, self.h.config, self.h)


if __name__ == '__main__':
    unittest.main()
