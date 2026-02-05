Pixi allows for reducible environments using conda packages.
It manages environments for all supported platforms together to help keep them in line with each other.

1. Follow the [pixi installation instructions](https://pixi.sh/latest/installation/) for your platform

1. Install the dependencies that are listed in the lockfile with

   ```sh
   pixi install --frozen
   ```

1. For in source builds, prefix all commands with `pixi run`. For example

   ```sh
   pixi run pre-commit install
   ```

1. For out-of source build, prefix all commands with

   ```sh
   pixi run --manifest-path path/to/source cmake --build .
   ```

1. Whenever `pixi.toml` changes, `pixi.lock` will update next time pixi is run.

When there are issues, one can often fix them by removing the `pixi.lock` and recreating it with any pixi command.

To not have to prefix all of the commands with `pixi run`, either use `pixi shell` (don't forget to `exit` when leaving the directory) or use [direnv](https://direnv.net/).
Once `direnv` is properly installed and in your path, create a file `.envrc` in your source tree with the contents

```sh
watch_file pixi.lock
eval "$(pixi shell-hook --frozen --change-ps1 false)"
```

For out of source builds, the build directory should have a `.envrc` with the contents

```sh
watch_file /path/to/source/mantid/pixi.lock
eval "$(pixi shell-hook --manifest-path=/path/to/source/mantid/ --frozen --change-ps1 false)"
export PYTHONPATH=/path/to/build/bin/:$PYTHONPATH
```

The last line is necessary to get the build results into the python path.
