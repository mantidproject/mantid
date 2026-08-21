"""Memory comparison between PA and RD instruments."""

import unittest
from mantid.simpleapi import LoadEmptyInstrument


class MemoryCheckTest(unittest.TestCase):
    def test_memory_comparison(self):
        ws_rd = LoadEmptyInstrument(Filename="SNAP_mini_RD_Definition.xml", OutputWorkspace="rd_mem")
        ws_pa = LoadEmptyInstrument(Filename="SNAP_mini_Definition.xml", OutputWorkspace="pa_mem")

        inst_rd = ws_rd.getInstrument()
        inst_pa = ws_pa.getInstrument()

        mem_rd = inst_rd.getMemorySize()
        mem_pa = inst_pa.getMemorySize()

        ci_rd = ws_rd.componentInfo()
        ci_pa = ws_pa.componentInfo()

        print(f"\nRD instrument memory: {mem_rd:>10,} bytes  ({mem_rd / 1024:.1f} kB)")
        print(f"PA instrument memory: {mem_pa:>10,} bytes  ({mem_pa / 1024:.1f} kB)")
        print(f"Reduction:            {mem_rd - mem_pa:>10,} bytes saved  ({100 * (mem_rd - mem_pa) / mem_rd:.1f}% smaller)")
        print(f"\nRD component tree: {ci_rd.size()} components")
        print(f"PA component tree: {ci_pa.size()} components")
        print(f"Component reduction: {ci_rd.size() - ci_pa.size()} fewer components")

        self.assertGreater(mem_rd, mem_pa, "PA instrument should use less memory than RD")


if __name__ == "__main__":
    unittest.main()
