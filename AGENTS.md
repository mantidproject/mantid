Mantid is a C++20 and Python 3.12 repository with a Qt GUI frontend.
It is configured with CMake and uses pixi/conda for dependencies.
The project provides tools for processing materials-science data from neutron scattering, muon spectroscopy, and simulation.

# Dev environment setup

- Install pixi and activate the environment specified in `pixi.toml`:
  - `pixi shell`
  - or `eval $(pixi shell-hook)` for non-interactive use
- Dependencies are tracked from a conda metapackage in `conda/recipes/mantid-developer`.

# Development workflow

- Inspect existing code, tests, and documentation before making changes.
- Keep changes focused on the requested behavior. Avoid unrelated refactors.
- Do not revert unrelated changes in the working tree.
- Prefer existing project patterns over introducing new abstractions.
- Avoid adding new dependencies unless they are necessary and justified.
- If a command fails, report the command, the relevant error, the likely cause, and the next step.
- Before summarizing work, check `git status` and review the relevant `git diff`.
- Final summaries should include changed files, tests run, tests not run, release note status, and documentation status.

# Configure and build

- Configure:
  - Linux: `cmake --preset=linux . && cd build/`
  - Windows: `cmake --preset=win-ninja . && cd build/`
  - macOS Apple Silicon: `cmake --preset=osx-arm64 . && cd build/`
- If unsure which preset to use, check `CMakePresets.json`.
- Build (run from the build directory):
  - Framework: `ninja Framework`
  - Workbench GUI: `ninja workbench`
  - Tests: `ninja AllTests`
- Mantid typically uses out of source builds. If build files related to ninja and cmake cannot be found, ask the user to
  grant access to the external build process.
- Ninja commands are typically ran from the build directory. If using pixi, you must specify the directory containing the `pixi.toml` file using the `--manifest-path` flag.
- Do not make direct changes to files in the build directory. These are generated as part of the build process.

# CMake

- Mantid uses CMake to define build targets and test targets.
- When adding, moving, or removing C++ source, header, test, or resource files, update the relevant `CMakeLists.txt`.
- Ensure new `src`, `inc`, `test`, and related files are included in the appropriate library, executable, or test target.

# Tests and debugging

Agents can run both unit and system tests for Python and C++ changes.

- When making code changes, add or update test coverage for the changed behavior.
- Prefer running the smallest relevant set of tests. The full test suite is large and can take a long time.
- For C++ changes, build the relevant test target and run the corresponding test executable from the build `bin` directory. Pass a test name or filter argument where the executable supports it.
- For Python changes, run the relevant Python unit test file or test case. Ensure `PYTHONPATH` includes the build `bin` directory so Mantid modules resolve correctly.
- For system-level behavior, run only matching system tests with:
  - `python Testing/SystemTests/scripts/runSystemTests.py -R <regex>`
- If `pixi shell` is not active, use `pixi run` to run tests, ensuring that conda packages are on the path. The Python executable from the active pixi environment can also be used.
- Run broader `ctest` or `./systemtest` commands only when the change has broad impact or focused tests are insufficient.
- Choose tests based on the changed behavior:
  - C++ algorithm changes usually need the relevant C++ unit test executable and any matching system test.
  - Python algorithm changes usually need the relevant Python unit test file or test case and any matching system test.
  - GUI changes usually need the relevant model, presenter, widget, or workflow tests.
- If the build environment is unavailable, state which tests should be run and why they were not run.
- If a test is changed to make it pass, explicitly explain why the changed expectation is valid.
- Unit test standards are documented in [Unit Test Standards](dev-docs/source/Standards/UnitTestStandards.rst).
- Coding standards are documented in [Mantid Standards](dev-docs/source/Standards/index.rst).
- Prefer targeted formatting or static checks for touched files first; run broader checks such as `pre-commit run --all-files` when practical.

# Documentation

- User documentation lives in [docs](docs/).
- Developer documentation lives in [dev-docs](dev-docs/).
- New Mantid algorithms must include user-facing algorithm documentation under [docs/source/algorithms](docs/source/algorithms/).
- Algorithm documentation requirements are described in [Algorithm Documentation](dev-docs/source/Standards/AlgorithmDocumentation.rst).
- General documentation guidance is in [Documentation Guide For Devs](dev-docs/source/Standards/DocumentationGuideForDevs.rst).
- For algorithm behavior or property changes, update both code-level summaries/property descriptions and the relevant `.rst` documentation.
- For GUI or workflow changes, update user documentation when users need to understand new or changed behavior.

# Release notes

- Release notes live under [docs/source/release](docs/source/release/).
- Follow the [Release Notes Guide](dev-docs/source/Standards/ReleaseNotesGuide.rst).
- Every user-facing change must include a release note unless the guide clearly says one is not required.
- Add release notes to the next release directory, using the relevant `New_features` or `Bugfixes` subdirectory.
- Write release notes for scientific users, not developers.
- Pure internal refactors, tests, or developer-documentation changes usually do not need release notes.

# PR checklist

- Run `clang-tidy` using the configuration in `.clang-tidy` where relevant.
- Format using `pre-commit run --all-files`.
- Follow the PR template in [.github/PULL_REQUEST_TEMPLATE.md](.github/PULL_REQUEST_TEMPLATE.md).
- Ensure relevant tests, docs, and release notes are included before review.
- Do not include generated build or documentation outputs unless the project explicitly requires them.

# Project structure

- `.github/workflows` build configuration for GitHub Actions.
- `buildconfig/Jenkins` build configuration for Jenkins CI.
- `buildconfig/CMake` custom CMake modules.
- `conda/recipes` recipes for conda packages.
- `dev-docs` developer documentation.
- `docs` user documentation and release notes.
- `Framework` non-GUI C++ and Python framework code.
- `instrument` XML files describing facilities, instruments, and sample environments.
- `qt` GUI code.
- `scripts` Python code.
- `Testing` test data and system test framework.
- `tools` small tools for maintaining code and report generation.
