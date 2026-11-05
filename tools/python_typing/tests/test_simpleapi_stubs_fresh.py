from __future__ import annotations

import subprocess
import sys
import unittest
from pathlib import Path


class SimpleAPIStubFreshnessTest(unittest.TestCase):
    def test_generated_stub_matches_registered_algorithms(self):
        repo_root = Path(__file__).resolve().parents[3]
        generator = repo_root / "tools/python_typing/generate_simpleapi_stubs.py"
        result = subprocess.run(
            [sys.executable, str(generator), "--all", "--check"],
            cwd=repo_root,
            capture_output=True,
            text=True,
            check=False,
        )
        self.assertEqual(result.returncode, 0, msg=result.stdout + result.stderr)


if __name__ == "__main__":
    unittest.main()
