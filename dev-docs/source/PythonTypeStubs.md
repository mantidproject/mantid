(PythonTypeStubs)=

# Python type stubs

This guide describes how to regenerate and validate Mantid's
`mantid.simpleapi` type stub. The stub is generated from registered algorithm
and property metadata in a configured Mantid build.

## Requirements

Use the Mantid developer environment and a completed build that can import
`mantid.simpleapi` and load the registered algorithms. The generator formats
its output with the repository's Ruff formatter through `prek` or
`pre-commit`; one of those commands must be available in the environment.

## Configure Pyright import paths

When checking code from a developer build, configure Pyright to search both the
Python source root and the build's binary directory. For a fixture saved in the
build directory, create a `pyrightconfig.json` there with paths for your
checkout and build:

```json
{
  "include": ["check_mantid_types.py"],
  "extraPaths": [
    "/path/to/mantid/Framework/PythonInterface",
    "/path/to/mantid-build/bin"
  ],
  "pythonVersion": "3.13",
  "typeCheckingMode": "basic"
}
```

`Framework/PythonInterface` contains the `mantid` package and
`simpleapi.pyi`; `mantid-build/bin` contains the compiled extension modules.
Use the matching Python environment when running Pyright or launching its
language server from an editor.

Python may import Mantid through a setuptools editable-install `.pth` file that
installs a custom import finder mapping `mantid` to the source tree. Pyright
does not use that Python import hook to find source modules, so both paths must
be configured directly in `extraPaths`.

## Generate the declarations

From the repository root, in the configured build environment, run:

```sh
python tools/python_typing/generate_simpleapi_stubs.py --all
```

This writes `Framework/PythonInterface/mantid/simpleapi.pyi`. To generate a
small subset while developing the generator, pass algorithm names instead of
`--all`. The committed file should be generated with `--all`.

Do not format the stub by hand. The generator runs Ruff on the generated file.
When formatting or output needs correction, update the generator or its
explicit wrapper-specific declarations, regenerate the file, and review the
resulting diff.

## Validate and review

Check that the committed stub matches the current build's registered
algorithms:

```sh
python tools/python_typing/generate_simpleapi_stubs.py --all --check
```

The Python typing tests are registered with CTest under the `UnitTest` label.
From the configured build directory, run the focused tests with:

```sh
ctest -R 'tools\.python_typing\.' --output-on-failure
```

The tests cover generator behavior, stub freshness, and an analyzer smoke check
for the `ConvertUnits` signature that requires Pyright to reject an invalid
`Target` value. The PR workflow runs these tests as part of its `UnitTest`
selection.

For an interactive fixture named `check_mantid_types.py` in the configured
build directory, run:

```sh
pyright check_mantid_types.py
```

`check` is not a Pyright subcommand. The command above uses the project's
`pyrightconfig.json` automatically when run from its directory.

Keep stub freshness and analyzer checks in CTest/CI. They need the Mantid
bindings and registered algorithms from the build; pre-commit remains
responsible for formatting and ordinary static checks.

When algorithm property metadata or simpleapi wrapper behavior changes, review
the generated declarations and update generator overrides and tests as needed.
The separate binding-stub workstream will document its own declaration source
and maintenance workflow for compiled `mantid.kernel`, `mantid.api`, and
`mantid.dataobjects` objects.
