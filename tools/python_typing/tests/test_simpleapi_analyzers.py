from __future__ import annotations

import json
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


class SimpleAPIAnalyzerSmokeTest(unittest.TestCase):
    def test_pyright_reads_convertunits_signature(self):
        repo_root = Path(__file__).resolve().parents[3]
        source_root = repo_root / "Framework/PythonInterface"
        build_paths = [path for path in sys.path if Path(path).is_dir() and any(Path(path).glob("_kernel*"))]
        fixture_text = "from mantid.simpleapi import ConvertUnits\n\nConvertUnits(InputWorkspace='input', Target='not-a-real-target')\n"
        marker = "not-a-real-target"

        with tempfile.TemporaryDirectory() as temporary_directory:
            temporary_path = Path(temporary_directory)
            fixture = temporary_path / "simpleapi_smoke.py"
            fixture.write_text(fixture_text)

            pyright = shutil.which("pyright")
            self.assertIsNotNone(pyright, "Pyright must be installed in the development environment")

            pyright_config = temporary_path / "pyrightconfig.json"
            pyright_config.write_text(
                json.dumps(
                    {
                        "include": [str(fixture)],
                        "extraPaths": [str(source_root), *build_paths],
                        "pythonVersion": "3.13",
                        "typeCheckingMode": "basic",
                        "reportMissingImports": "none",
                        "reportMissingModuleSource": "none",
                    }
                )
            )
            pyright_result = subprocess.run(
                [pyright, "--project", str(pyright_config)],
                capture_output=True,
                text=True,
                check=False,
            )

        output = pyright_result.stdout + pyright_result.stderr
        self.assertNotEqual(pyright_result.returncode, 0, "Pyright accepted an invalid Target value")
        self.assertIn(marker, output, f"Pyright did not report the invalid Target value:\n{output}")


if __name__ == "__main__":
    unittest.main()
