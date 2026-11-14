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

From the build directory, run:

```sh
ninja simpleapi_stubs
```

This builds the framework libraries and Python plugins that register
algorithms, then writes `Framework/PythonInterface/mantid/simpleapi.pyi` in the
source tree. The target is not part of the default build because it changes a
committed file. It is re-run only when a framework library, Python extension
module, Python algorithm, the `mantid` package, or the generator has changed.
It runs the generator with an empty Mantid user properties directory, so local
settings such as `default.facility` do not end up in the committed stub.
Because the stub is the target's output, `ninja clean` deletes it; restore it
with `git checkout` or by running the target again.

To run the generator directly instead, from the repository root in the
configured build environment, with the build's `bin` directory on
`PYTHONPATH`:

```sh
python tools/python_typing/generate_simpleapi_stubs.py --all
```

Settings in your `Mantid.user.properties` affect the output of a direct run.
To generate a small subset while developing the generator, pass algorithm names
instead of `--all`. The committed file should be generated with `--all`.

Algorithms in the `MPI` category (`BroadcastWorkspace`, `GatherWorkspaces`)
are left out of `simpleapi.pyi`, because the `MPIAlgorithms` plugin is only
built on Linux and including them would make the committed stub depend on
where it was generated. Instead, `simpleapi.pyi` star-imports
`mantid._simpleapi_mpi`, a stub that `--mpi-only` writes to
`Framework/PythonInterface/mantid/_simpleapi_mpi.pyi`. `ninja simpleapi_stubs`
generates it wherever the plugin is built. It is ignored by git and shipped
only in the `mantidmpi` conda package. Where it is missing, type checkers
resolve the MPI algorithms through the stub's `__getattr__` fallback as `Any`.

Do not format the stub by hand. The generator runs Ruff on the generated file.
When formatting or output needs correction, update the generator or its
explicit wrapper-specific declarations, regenerate the file, and review the
resulting diff.

## Packages and the committed stub

The conda package build regenerates both stubs from the build it is packaging,
by running the `simpleapi_stubs` target before install with
`-DSIMPLEAPI_STUBS_ARGS=--no-format`. Packaged stubs therefore always match
their algorithms. `--no-format` skips Ruff formatting, which needs `prek` or
`pre-commit` and a git checkout. Formatting matters only for the reviewed diff
of the committed file.

The committed stub is not checked for freshness in CI, so pull requests that
change algorithm properties do not need to regenerate it, and feature branches
do not conflict over it. It may lag behind the algorithms on `main` until
someone regenerates it. To check whether it is stale, run:

```sh
python tools/python_typing/generate_simpleapi_stubs.py --all --check
```

## Validate and review

The Python typing tests are registered with CTest under the `UnitTest` label.
From the configured build directory, run the focused tests with:

```sh
ctest -R 'tools\.python_typing\.' --output-on-failure
```

The tests cover generator behavior and an analyzer smoke check for the
`ConvertUnits` signature that requires Pyright to reject an invalid
`Target` value. The PR workflow runs these tests as part of its `UnitTest`
selection.

For an interactive fixture named `check_mantid_types.py` in the configured
build directory, run:

```sh
pyright check_mantid_types.py
```

`check` is not a Pyright subcommand. The command above uses the project's
`pyrightconfig.json` automatically when run from its directory.

Keep analyzer checks in CTest/CI. They need the Mantid bindings and
registered algorithms from the build; pre-commit remains responsible for
formatting and ordinary static checks.

When algorithm property metadata or simpleapi wrapper behavior changes, review
the generated declarations and update generator overrides and tests as needed.
The separate binding-stub workstream will document its own declaration source
and maintenance workflow for compiled `mantid.kernel`, `mantid.api`, and
`mantid.dataobjects` objects.
