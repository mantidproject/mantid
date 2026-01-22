# Using Local Mantid Build in Other Projects with Pixi

```{contents}
:local:
```

## Overview

This guide shows how to configure a pixi environment to use your local
Mantid build instead of the conda package in other projects that depend
on Mantid. This is useful when you need to develop and test changes
across both Mantid and dependent applications simultaneously.

## Prerequisites

- A successfully built local Mantid installation (see
  `BuildingWithCMake`)
- [Pixi](https://pixi.ws/latest/) installed
- Your dependent project with pixi configuration

## Configuration

Add a new pixi environment to your project's `pyproject.toml` file:

``` toml
[tool.pixi.environments]
local-mantid = { features = ["local-mantid"], solve-group = "default" }

[tool.pixi.feature.local-mantid.dependencies]
# Include your project's dependencies but exclude mantid packages
python = ">=3.10"
numpy = "*"
# ... other dependencies (do NOT include mantid, mantidworkbench, mantidqt)

[tool.pixi.feature.local-mantid.activation.env]
MANTID_BUILD_DIR = "/absolute/path/to/your/mantid/build"
MANTID_SOURCE_DIR = "/absolute/path/to/your/mantid/source"
PYTHONPATH = "${MANTID_BUILD_DIR}/bin:${MANTID_SOURCE_DIR}/Framework/PythonInterface:${MANTID_SOURCE_DIR}/qt/python/mantidqt"
LD_LIBRARY_PATH = "${MANTID_BUILD_DIR}/bin"        # Linux
DYLD_LIBRARY_PATH = "${MANTID_BUILD_DIR}/bin"      # macOS
MANTIDPATH = "${MANTID_BUILD_DIR}/bin"

[tool.pixi.feature.local-mantid.tasks]
# Launch local Mantid Workbench with your application
launch-local-workbench = { cmd = "cd $MANTID_BUILD_DIR/bin && ./launch_mantidworkbench.sh", description = "Launch local Mantid Workbench with your application available" }
```

Replace the paths with your actual Mantid build and source directories.

## Usage

1.  Install the environment:

    ``` bash
    pixi install --environment local-mantid
    ```

2.  Run your application:

    ``` bash
    pixi run --environment local-mantid your-command
    ```

3.  Run Local Workbench in your application's context:

    ``` bash
    pixi run --environment local-mantid launch-local-workbench
    ```

## Troubleshooting

**Import errors**: Verify `PYTHONPATH` includes
`${MANTID_BUILD_DIR}/bin` and your Mantid build completed successfully.

**Library loading errors**: Check that `LD_LIBRARY_PATH` (Linux) or
`DYLD_LIBRARY_PATH` (macOS) includes `${MANTID_BUILD_DIR}/bin`.

**Path issues**: Ensure all paths are absolute. Use `realpath` to
resolve symbolic links if needed.

You can verify the environment setup with:

``` bash
pixi run --environment local-mantid python -c "
import os, mantid
print('Mantid version:', mantid.__version__)
print('Mantid path:', mantid.__file__)
print('Build dir:', os.environ.get('MANTID_BUILD_DIR'))
"
```

## Related Documentation

- `BuildingWithCMake` - Building Mantid from source
- `GettingStarted` - Initial setup for Mantid development
- [Pixi Documentation](https://pixi.ws/latest/) - Complete pixi
  reference
