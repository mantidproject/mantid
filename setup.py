from setuptools import setup
import versioningit


def get_version():
    """Return the full version string.

    This is called by CMake VersionNumber.cmake & setup() below
    to ensure the same string is used in both cases.
    """
    # Use versioningit if available to generate full information
    # for cmake build.
    return versioningit.get_version()


if __name__ == "__main__":
    # Minimal setuptools setup to allow calling `load_setup_py_data`
    # from conda meta.yaml
    setup(version=get_version())
