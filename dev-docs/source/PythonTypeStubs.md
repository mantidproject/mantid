(PythonTypeStubs)=

# Python type stubs

This guide describes how to generate and validate Mantid's
`mantid.simpleapi` type stub. The stub is a build output: it is generated from
registered algorithm and property metadata in a configured Mantid build, and
is not committed to git.

## Requirements

Use the Mantid developer environment and a completed build that can import
`mantid.simpleapi` and load the registered algorithms.

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

`Framework/PythonInterface` contains the `mantid` package and, once generated,
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
source tree, next to `simpleapi.py`, where git ignores it. The target is not
part of the default build, because regenerating the stub after every relink
would add seconds to each build; run it again when you want your editor to see
changed algorithm signatures. Configure with `-DSIMPLEAPI_STUBS_IN_ALL=ON` to
add it to the default build instead, as the CI presets in `CMakePresets.json`
do (apart from the sanitizer presets). It is re-run only when a framework library,
Python extension module, Python algorithm, the `mantid` package, or the
generator has changed. It runs the generator with an empty Mantid user
properties directory, so local settings such as `default.facility` do not end
up in the stub. `ninja clean` deletes the stub.

Because the stub is written into the source tree, build directories that share
one checkout (for example a Release and a Debug build) share one stub: each
overwrites the other's, and `ninja clean` in one deletes it for both. With
`SIMPLEAPI_STUBS_IN_ALL` off, `ninja install` packages whatever stub is in the
source tree, which may be stale or missing; run `ninja simpleapi_stubs` first.

To run the generator directly instead, from the repository root in the
configured build environment, with the build's `bin` directory on
`PYTHONPATH`:

```sh
python tools/python_typing/generate_simpleapi_stubs.py --all
```

Settings in your `Mantid.user.properties` affect the output of a direct run.
To generate a small subset while developing the generator, pass algorithm names
instead of `--all`.

Algorithms in the `MPI` category (`BroadcastWorkspace`, `GatherWorkspaces`)
are left out of `simpleapi.pyi`, because the `MPIAlgorithms` plugin is only
built on Linux and including them would make the stub depend on where it was
generated. Instead, `simpleapi.pyi` star-imports
`mantid._simpleapi_mpi`, a stub that `--mpi-only` writes to
`Framework/PythonInterface/mantid/_simpleapi_mpi.pyi`. `ninja simpleapi_stubs`
generates it wherever the plugin is built. It is shipped only in the
`mantidmpi` conda package. Where it is missing, type checkers
resolve the MPI algorithms through the stub's `__getattr__` fallback as `Any`.

When output needs correction, update the generator or its explicit
wrapper-specific declarations and regenerate the file.

## Packages

The conda package build generates both stubs from the build it is packaging,
by configuring with `-DSIMPLEAPI_STUBS_IN_ALL=ON`, so they are in place
before install. Packaged stubs
therefore always match their algorithms.

Generating the stub imports every Python algorithm plugin, so the staging
`host` requirements in `conda/recipes/mantid/recipe.yaml` must include the
Python packages that plugins import when they load. Required packages are also
in the `mantid` run requirements. Optional packages that decide whether an
algorithm is registered, such as `pystog`, are only in `host` and
`run_constraints`. `pystog` comes from the `neutrons` channel, which
`buildconfig/Jenkins/Conda/package-conda` passes to the `mantid` build. If a
plugin fails to load, the generator stops with a list of the failed files
rather than writing a stub without their algorithms. An optional import that
only skips registering an algorithm cannot be detected; that algorithm is then
missing from the packaged stub until its package is added to `host`.

Because the stub is not committed, pull requests that change algorithm
properties never need to update it, and branches cannot disagree about it.

## Validate and review

The Python typing tests are registered with CTest under the `UnitTest` label.
From the configured build directory, run the focused tests with:

```sh
ctest -R 'tools\.python_typing\.' --output-on-failure
```

The tests cover generator behavior and an analyzer smoke check for the
`ConvertUnits` signature that requires Pyright to reject an invalid
`Target` value. The smoke check reads the generated stub, so CTest runs the
`simpleapi_stubs` target first (a CTest fixture). This is quick when the stub
is up to date, but on an unbuilt or stale build directory it first builds
every framework library and Python module the stub depends on, which can take
a long time. The PR workflow runs these tests as part of its `UnitTest`
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
