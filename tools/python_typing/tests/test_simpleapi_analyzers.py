from __future__ import annotations

import json
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


class SimpleAPIAnalyzerSmokeTest(unittest.TestCase):
    def test_ty_and_pyright_read_convertunits_signature(self):
        repo_root = Path(__file__).resolve().parents[3]
        source_root = repo_root / "Framework/PythonInterface"
        build_paths = [path for path in sys.path if Path(path).is_dir() and any(Path(path).glob("_kernel*"))]
        fixture_text = "from mantid.simpleapi import ConvertUnits\n\nConvertUnits(InputWorkspace='input', Target='not-a-real-target')\n"
        marker = "not-a-real-target"

        with tempfile.TemporaryDirectory() as temporary_directory:
            temporary_path = Path(temporary_directory)
            fixture = temporary_path / "simpleapi_smoke.py"
            fixture.write_text(fixture_text)

            ty = shutil.which("ty")
            pyright = shutil.which("pyright")
            self.assertIsNotNone(ty, "ty must be installed in the development environment")
            self.assertIsNotNone(pyright, "Pyright must be installed in the development environment")

            ty_result = subprocess.run(
                [
                    ty,
                    "check",
                    "--python",
                    sys.executable,
                    "--extra-search-path",
                    str(source_root),
                    *sum((["--extra-search-path", path] for path in build_paths), []),
                    "--ignore",
                    "import-not-found",
                    str(fixture),
                ],
                capture_output=True,
                text=True,
                check=False,
            )
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

        for analyzer, result in (("ty", ty_result), ("Pyright", pyright_result)):
            output = result.stdout + result.stderr
            self.assertNotEqual(result.returncode, 0, f"{analyzer} accepted an invalid Target value")
            self.assertIn(marker, output, f"{analyzer} did not report the invalid Target value:\n{output}")


if __name__ == "__main__":
    unittest.main()
