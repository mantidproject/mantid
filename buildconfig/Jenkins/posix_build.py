#!/bin/python3

# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

###############################################################################
# LINUX/MAC SCRIPT TO DRIVE THE JENKINS BUILDS OF MANTID.
#
# Notes:
#
# WORKSPACE, JOB_NAME, NODE_LABEL, PACKAGE_SUFFIX, GIT_COMMIT are
# environment variables that are set by Jenkins. The last one
# corresponds to any labels set on a slave.  BUILD_THREADS should
# be set in the configuration of each slave.

###############################################################################
# Set all string comparisons to case insensitive (i.e. Release == release)

# shopt -s nocasematch
import os
import pathlib
import resource
import shutil
import subprocess
import sys
from shutil import which


def _run_command(command: str, exceptions=True, **kwargs) -> subprocess.CompletedProcess:
    print(f"+{command}", flush=True)

    parsed_command = command.split(' ')
    parsed_command = list(filter(None, parsed_command))
    return subprocess.run(parsed_command, check=exceptions, universal_newlines=True, **kwargs)


def _tool_exists(tool_name: str) -> bool:
    return which(tool_name) is not None


def _env_var(var_name: str) -> str:
    var = os.environ.get(var_name, default="")
    off_attrs = ["off", "false", "none", ""]
    if any(var.casefold() == off_atr for off_atr in off_attrs):
        return ""  # Bash default for unitialised attr
    return var


SCRIPT_DIR=pathlib.Path(__file__).parent
XVFB_SERVER_NUM=101

ULIMIT_CORE_ORIG=resource.getrlimit(resource.RLIMIT_CORE)

###############################################################################
# Functions
###############################################################################


def on_exit():
    if _env_var("USE_CORE_DUMPS"):
        resource.setrlimit(resource.RLIMIT_CORE, ULIMIT_CORE_ORIG)


def run_with_xvfb(command: str, **kwargs):
    if not _tool_exists("xvfb-run"):
        return _run_command(command, **kwargs)
    else:
        parsed_command = command.split(' ')
        parsed_command = list(filter(None, parsed_command))

        # Manually split so our server args don't get mangled
        xvfb_template = ["xvfb-run", "-e", "/dev/stderr", "--server-args=-core -noreset -screen 0 640x480x24",
                         f"--server-num={XVFB_SERVER_NUM}"]
        xvfb_template.extend(parsed_command)
        print(f"+{' '.join(xvfb_template)}", flush=True)
        return subprocess.run(xvfb_template, check=True, universal_newlines=True, **kwargs)


def terminate_xvfb_sessions():
    if _tool_exists("xvfb-run"):
        print("Terminating existing Xvfb sessions")
        _run_command("killall Xvfb", exceptions=False)
        lock_path = pathlib.PosixPath(f"/tmp/.X{XVFB_SERVER_NUM}-lock").resolve()
        try:
            lock_path.unlink()
        except FileNotFoundError:
            pass


def main():  # noqa: C901
    ###############################################################################
    # Terminate existing Xvfb sessions
    ###############################################################################
    terminate_xvfb_sessions()

    ###############################################################################
    # System discovery
    ###############################################################################
    USE_CORE_DUMPS=True
    ON_RHEL7=False
    ON_UBUNTU=False
    ON_MACOS=False

    NODE_LABELS = _env_var("NODE_LABELS").lower()
    if "rhel7" in NODE_LABELS or "centos7" in NODE_LABELS  or "scilin7" in NODE_LABELS:
        ON_RHEL7=True
    elif "ubuntu" in NODE_LABELS:
        ON_UBUNTU=True
    elif "osx" in NODE_LABELS:
        ON_MACOS=True
        USE_CORE_DUMPS=False

    ###############################################################################
    # Script cleanup
    ###############################################################################
    # We rely on exceptions to kill Python in _run_command instead
    # trap onexit INT TERM EXIT

    ###############################################################################
    # Some preprocessing steps on the workspace
    ###############################################################################
    WORKSPACE = _env_var("WORKSPACE")
    if not WORKSPACE:
        raise OSError("WORKSPACE ENV NOT SET")
    BUILD_DIR_REL="build"
    BUILD_DIR=pathlib.PosixPath(WORKSPACE).resolve() / BUILD_DIR_REL

    # Clean the source tree to remove stale configured files but make sure to
    # leave external/ and the BUILD_DIR directory intact.
    # There is a later check to see if this is a clean build and remove BUILD_DIR.
    _run_command(f"git clean -d -x --force --exclude={BUILD_DIR_REL} --exclude=\".Xauthority-*\"")

    ###############################################################################
    # Print out the versions of things we are using
    ###############################################################################
    # we use cmake3 on rhel because cmake is too old
    if _tool_exists("cmake3"):
        CMAKE_EXE="cmake3"
        CPACK_EXE="cpack3"
        CTEST_EXE="ctest3"
    else:
        CMAKE_EXE="cmake"
        CPACK_EXE="cpack"
        CTEST_EXE="ctest"
    _run_command(f"{CMAKE_EXE} --version")

    ###############################################################################
    # Check job requirements from the name and changes
    ###############################################################################
    JOB_NAME = _env_var("JOB_NAME").lower()
    if "clean" in JOB_NAME or "clang_tidy" in JOB_NAME:
        CLEANBUILD=True
    else:
        CLEANBUILD=False

    if "coverage" in JOB_NAME:
        COVERAGE="ON"
    else:
        COVERAGE="OFF"

    if  "pull_requests" in JOB_NAME:
        PRBUILD=True
    else:
        PRBUILD=False

    if  "debug" in JOB_NAME:
        BUILD_CONFIG="Debug"
    elif "relwithdbg" in JOB_NAME:
        BUILD_CONFIG="RelWithDbg"
    else:
        BUILD_CONFIG="Release"

    # For pull requests decide on what to build based on changeset and Jenkins
    # parameters.
    DO_BUILD_CODE=True
    DO_UNITTESTS=True
    DO_DOCTESTS_USER=True
    DO_BUILD_DEVDOCS=True
    DO_BUILD_PKG=True
    DO_SYSTEMTESTS=False

    cfc_exec = SCRIPT_DIR / "check_for_changes"
    cfc_exec = str(cfc_exec.resolve(strict=True))
    if PRBUILD:
        if _run_command((cfc_exec + " dev-docs-only"), exceptions=False).returncode == 0 and \
           _run_command((cfc_exec + " user-docs-only"), exceptions=False).returncode == 0:
            DO_BUILD_CODE=False
            DO_UNITTESTS=False
        DO_DOCTESTS_USER=False
        DO_BUILD_DEVDOCS=False  # noqa: F841
        build_pkg = True if _env_var("BUILD_PACKAGE") else False
        DO_BUILD_PKG=build_pkg
        DO_SYSTEMTESTS=False

    if ON_RHEL7:
        # rhel does system testing if there are any non-doc or gui changes
        if not _run_command((cfc_exec + " docs-gui-only"), exceptions=False).returncode == 0:
            DO_BUILD_PKG=True
            DO_SYSTEMTESTS=True
    elif ON_UBUNTU:
        # ubuntu does the docs build
        if _run_command((cfc_exec + " dev-docs-only"), exceptions=False).returncode == 0:
            DO_BUILD_CODE=False
            DO_DOCTESTS_USER=False
        else:
            DO_BUILD_CODE=True # code needs to be up to date
            DO_DOCTESTS_USER=True

    ###############################################################################
    # Setup the build directory
    # For a clean build the entire thing is removed to guarantee it is clean. All
    # other build types are assumed to be incremental and the following items
    # are removed to ensure stale build objects don't interfere with each other:
    #   - build/bin/**: if libraries are removed from cmake they are not deleted
    #                   from bin and can cause random failures
    #   - build/ExternalData/**: data files will change over time and removing
    #                            the links helps keep it fresh
    #   - build/Testing/**: old ctest xml files will change over time and removing
    #                       the links helps keep it fresh
    ###############################################################################
    if not BUILD_DIR:
        print("Build directory not set. Cannot continue", file=sys.stderr)
        sys.exit(1)

    if _env_var("CLEANBUILD"):
        shutil.rmtree(BUILD_DIR)

    BUILD_DIR.mkdir(exist_ok=True)

    # Tidy build dir
    shutil.rmtree((BUILD_DIR / "bin"), ignore_errors=True)
    shutil.rmtree((BUILD_DIR / "ExternalData"), ignore_errors=True)
    shutil.rmtree((BUILD_DIR / "Testing"), ignore_errors=True)
    _run_command(f"find {BUILD_DIR} ( -name 'TEST-*.xml' -o -name 'Test.xml' ) -delete")

    if  _env_var("CLEAN_EXTERNAL_PROJECTS"):
        shutil.rmtree((BUILD_DIR / "_deps"), ignore_errors=True)

    ###############################################################################
    # Setup clang
    ###############################################################################
    USE_CLANG = False
    if  "clang" in JOB_NAME:
        USE_CLANG=True
    elif  ON_MACOS:
        if not _tool_exists("icpc"):
            USE_CLANG=True

    CC=""
    CXX=""
    CLANGTIDYVAR=""
    if USE_CLANG:
        # Assuming we are using the clang compiler
        print("Using clang/llvm compiler.")
        _run_command("clang --version")
        CC="clang"
        CXX="clang++"

        # check if this is also a clang-tidy build
        if  "clang_tidy" in JOB_NAME:
            CLANGTIDYVAR="-DENABLE_CLANG_TIDY=ON"

        #check if CMakeCache.txt exists and if so that the cxx compiler is clang++
        #only needed with incremental builds. Clean builds delete this directory in a later step.
        if (BUILD_DIR/ "CMakeCache.txt").exists() and "clean" in JOB_NAME:
            COMPILERFILEPATH= \
                _run_command(f"grep 'CMAKE_CXX_COMPILER:FILEPATH' {str(BUILD_DIR/ 'CMakeCache.txt')}").stdout
            if "clang++" not in COMPILERFILEPATH:
                # Removing the build directory entirely guarantees clang is used.
                shutil.rmtree(BUILD_DIR)

    #for openmp support on OS X run
    # `brew install llvm`
    # `ln -s /usr/local/opt/llvm/lib/libomp.dylib /usr/local/lib/libomp.dylib`
    if  ON_MACOS:
        if  "openmp" in JOB_NAME:
            CC="/usr/local/opt/llvm/bin/clang"
            CXX="/usr/local/opt/llvm/bin/clang++"

    ###############################################################################
    # Set up the location for the local object store outside of the build and
    # source tree, which can be shared by multiple builds.
    # It defaults to a MantidExternalData directory within the HOME directory.
    # It can be overridden by setting the MANTID_DATA_STORE environment variable.
    ###############################################################################
    MANTID_DATA_STORE =  _env_var("MANTID_DATA_STORE")
    if not MANTID_DATA_STORE:
        MANTID_DATA_STORE=_env_var("HOME") + "/MantidExternalData"

    ###############################################################################
    # Packaging options
    ###############################################################################
    PACKAGINGVARS=""
    if DO_BUILD_PKG:
        PACKAGINGVARS="-DPACKAGE_DOCS=ON -DDOCS_DOTDIAGRAMS=ON -DDOCS_SCREENSHOTS=ON" \
                      " -DDOCS_MATH_EXT=sphinx.ext.imgmath -DDOCS_PLOTDIRECTIVE=ON"

    # Use different suffix if parameter is not defined
    if _env_var("PACKAGE_SUFFIX"):
        PACKAGE_SUFFIX = _env_var("PACKAGE_SUFFIX")
        print(f"Using PACKAGE_SUFFIX={PACKAGE_SUFFIX} from job parameter")
    elif JOB_NAME.startswith("release"):  # starts with "release"
        PACKAGE_SUFFIX=""
    elif  JOB_NAME == "ornl-stable":
        PACKAGE_SUFFIX=""
    elif  JOB_NAME == "ornl-qa":
        PACKAGE_SUFFIX="qa"
    elif  JOB_NAME.startswith("master"):
        PACKAGE_SUFFIX="nightly"
    else:
        PACKAGE_SUFFIX="unstable"

    if  ON_MACOS:
        PACKAGINGVARS=f"{PACKAGINGVARS} -DCPACK_PACKAGE_SUFFIX={PACKAGE_SUFFIX}"
    else:
        if  JOB_NAME.startswith("release") and not PACKAGE_SUFFIX:
            # Traditional install path for release build
            PACKAGINGVARS=f"{PACKAGINGVARS} -DCMAKE_INSTALL_PREFIX=/opt/Mantid -DCPACK_PACKAGE_SUFFIX="
        elif JOB_NAME == "ornl-stable" and not PACKAGE_SUFFIX:
            # Traditional install path for release build
            PACKAGINGVARS=f"{PACKAGINGVARS} -DCMAKE_INSTALL_PREFIX=/opt/Mantid -DCPACK_PACKAGE_SUFFIX="
        else:
            # everything else uses lower-case values
            PACKAGINGVARS=f"{PACKAGINGVARS} -DCMAKE_INSTALL_PREFIX=/opt/mantid{PACKAGE_SUFFIX} -DCPACK_PACKAGE_SUFFIX={PACKAGE_SUFFIX}"

        RELEASE_NUMBER = _env_var("RELEASE_NUMBER")
        if  ON_RHEL7:
            if not RELEASE_NUMBER:
                RELEASE_NUMBER="1"
            PACKAGINGVARS=f"{PACKAGINGVARS} -DCPACK_RPM_PACKAGE_RELEASE={RELEASE_NUMBER}"

        GITHUB_AUTH_TOKEN = _env_var("GITHUB_AUTHORIZATION_TOKEN")
        if  GITHUB_AUTH_TOKEN:
            PACKAGINGVARS=f"{PACKAGINGVARS} -DGITHUB_AUTHORIZATION_TOKEN={GITHUB_AUTH_TOKEN}"

    ###############################################################################
    # Figure out if were doing a sanitizer build and setup any steps we need
    ###############################################################################
    SANITIZER_FLAGS=""
    if  "address" in JOB_NAME:
        SANITIZER_FLAGS="-DUSE_SANITIZER=Address"
    elif  "memory" in JOB_NAME:
        SANITIZER_FLAGS="-DUSE_SANITIZER=memory"
    elif  "thread" in JOB_NAME:
        SANITIZER_FLAGS="-DUSE_SANITIZER=thread"
    elif  "undefined" in JOB_NAME:
        SANITIZER_FLAGS="-DUSE_SANITIZER=undefined"

    if  SANITIZER_FLAGS:
        # Force build to RelWithDebInfo
        BUILD_CONFIG="RelWithDebInfo"

    ###############################################################################
    # Generator
    ###############################################################################
    CMAKE_GENERATOR = ""
    if _tool_exists("ninja"):
        CMAKE_GENERATOR="-G Ninja"
    elif _tool_exists("ninja-build"):
        CMAKE_GENERATOR="-G Ninja"

    if (BUILD_DIR/ "CMakeCache.txt").exists():
        CMAKE_GENERATOR=""

    ###############################################################################
    # Work in the build directory
    ###############################################################################
    os.chdir(str(BUILD_DIR))

    ###############################################################################
    # Clean up any artifacts from last build so that if it fails
    # they don't get archived again
    ###############################################################################
    for ext in ["*.dmg",  "*.rpm",  "*.deb", "*.tar.gz", "*.tar.xz"]:
        for file in BUILD_DIR.glob(ext):
            file.unlink()

    ###############################################################################
    # CMake configuration
    ###############################################################################
    DIST_FLAGS = _env_var("DIST_FLAGS") if _env_var("DIST_FLAGS") else ""

    host_env = os.environ.copy()
    host_env["CC"] = CC
    host_env["CXX"] = CXX

    _run_command(
        f"{CMAKE_EXE} {CMAKE_GENERATOR} -DCMAKE_BUILD_TYPE={BUILD_CONFIG}"
        f" -DENABLE_PRECOMMIT=OFF -DCOVERAGE={COVERAGE} -DENABLE_CPACK=ON"
        f" -DENABLE_MANTIDPLOT=OFF -DMANTID_DATA_STORE={MANTID_DATA_STORE}"
        " -DDOCS_HTML=ON -DENABLE_CONDA=ON -DCOLORED_COMPILER_OUTPUT=OFF "
        f" {DIST_FLAGS} {PACKAGINGVARS} {CLANGTIDYVAR} {SANITIZER_FLAGS}"
        " ..",
        env=host_env,
        cwd=BUILD_DIR
    )
    ###############################################################################
    # Coverity build should exit early
    ###############################################################################
    # TODO remove this block

    # if  "coverity_build_and_submit" in JOB_NAME:
    # ${COVERITY_DIR}/cov-build --dir cov-int ${CMAKE_EXE} --build . -- -j ${BUILD_THREADS:?}
    # tar czvf mantid.tgz cov-int
    # status=$(curl --form token=$COVERITY_TOKEN --form email=mantidproject@gmail.com \
    #     --form file=@mantid.tgz --form version=$GIT_COMMIT \
    #     https://scan.coverity.com/builds?project=mantidproject%2Fmantid)
    # status=$(echo ${status} | sed -e 's/^ *//' -e 's/ *$//')
    # if  -z $status ]] ||  ${status} == "Build successfully submitted.":
    # exit 0
    # else
    # echo "$status"
    # exit 1
    # fi
    # fi

    ###############################################################################
    # Build step
    ###############################################################################
    BUILD_THREADS = _env_var("BUILD_THREADS") if _env_var("BUILD_THREADS") else 0
    if DO_BUILD_CODE:
        _run_command(f"{CMAKE_EXE} --build {BUILD_DIR} -- -j{BUILD_THREADS}")
        _run_command(f"{CMAKE_EXE} --build {BUILD_DIR} --target AllTests -- -j{BUILD_THREADS}")

    ###############################################################################
    # Static analysis builds or stop here
    ###############################################################################
    if  USE_CLANG and "clang_tidy" in JOB_NAME:
        sys.exit(0)

    ###############################################################################
    # Run the unit tests
    ###############################################################################
    # Activate core dumps. They are deactivated by the registered EXIT function
    # at the top of this script
    if  USE_CORE_DUMPS:
        # Set ulimit to unlimited
        resource.setrlimit(resource.RLIMIT_CORE, (resource.RLIM_INFINITY, resource.RLIM_INFINITY))

    # Prevent race conditions when creating the user config directory
    userconfig_dir = pathlib.PosixPath.home() / ".mantid"
    shutil.rmtree(userconfig_dir, ignore_errors=True)

    # Remove old application saved state & crash reports on mac.
    # If we don't do this then when a previous test has crashed macOS
    # will pop up a dialog box and wait for clicking okay. We are heavy
    # handed but builders tend not to be used for anything else.
    if  ON_MACOS:
        shutil.rmtree((pathlib.PosixPath.home() / "Library/Saved Application State/org.python.python"), ignore_errors=True)
        shutil.rmtree((pathlib.PosixPath.home() / "Library/Application Support/CrashReporter"), ignore_errors= True)

    # Remove GUI qsettings files
    try:
        pathlib.PosixPath("~/.config/mantidproject/mantidworkbench.ini").resolve().unlink()
    except FileNotFoundError:
        pass

    userconfig_dir.mkdir(parents=True)
    # use a fixed number of openmp threads to avoid overloading the system
    userprops_file= userconfig_dir / "Mantid.user.properties"
    userprops_file.touch()
    with open(str(userprops_file.resolve()), mode='a') as props_file:
        props_file.write("MultiThreaded.MaxCores=2")

    if DO_UNITTESTS:
        run_with_xvfb(f"{CTEST_EXE} --no-compress-output -T Test -j{BUILD_THREADS} "
                      "--schedule-random --output-on-failure", cwd=BUILD_DIR)
        terminate_xvfb_sessions()

    ###############################################################################
    # User Documentation
    ###############################################################################
    if  DO_DOCTESTS_USER:
        # use default configuration
        userprops_file.unlink()
    # Remove doctrees directory so it forces a full reparse. It seems that
    # without this newly added doctests are not executed
    doc_trees_dir = BUILD_DIR / "docs/doctrees"
    if doc_trees_dir.exists():
        shutil.rmtree(doc_trees_dir, ignore_errors=True)

    # Build HTML to verify that no referencing errors have crept in.
    run_with_xvfb(f"{CMAKE_EXE} --build {BUILD_DIR} --target docs-html")
    terminate_xvfb_sessions()
    run_with_xvfb(f"{CMAKE_EXE} --build {BUILD_DIR} --target docs-doctest")

    ###############################################################################
    # Developer Documentation
    ###############################################################################
    # Uncomment this when the dev-docs are ready to build without warnings
    # if  ${DO_BUILD_DEVDOCS} == True:
    #   if [ -d $BUILD_DIR/dev-docs/doctree ]; then
    #     rm -fr $BUILD_DIR/dev-docs/doctree/*
    #   fi
    #   ${CMAKE_EXE} --build . --target dev-docs-html
    # fi

    ###############################################################################
    # Create the install kit if required. This includes building the Qt help
    # documentation
    ###############################################################################
    if  DO_BUILD_PKG:
        run_with_xvfb(f"{CMAKE_EXE} --build {BUILD_DIR} --target docs-qthelp")
        _run_command(f"{CPACK_EXE}")

        # Source tarball on clean build (arbitrarily choose Ubuntu)
        # Also, parcel up the documentation into a tar file that is easier to move around
        # and labelled by the commit id it was built with. This assumes the Jenkins git plugin
        # has set the GIT_COMMIT environment variable
        if CLEANBUILD and ON_UBUNTU:
            run_with_xvfb(f"{CMAKE_EXE} --build {BUILD_DIR} --target docs-html")
        GIT_COMMIT = _env_var("GIT_COMMIT")[0:7]
        _run_command(f"tar -cjf mantiddocs-g{GIT_COMMIT}.tar.bz2 --exclude='*.buildinfo' --exclude=\"MantidProject.q*\" docs/html")

        # The ..._PREFIX argument avoids opt/Mantid directories at the top of the tree
        _run_command(f"{CPACK_EXE} --config CPackSourceConfig.cmake -D CPACK_PACKAGING_INSTALL_PREFIX=")

    ###############################################################################
    # Run the system tests if required. Run from a package to have at least one
    # Linux checks it install okay
    ###############################################################################
    if DO_SYSTEMTESTS:
        system_tests_file = str((SCRIPT_DIR / "systemtests").resolve(strict=True))
        if PRBUILD:
            host_env["EXTRA_ARGS"] = "--exclude-in-pull-requests"
        _run_command(system_tests_file, env=host_env, shell=True)


if __name__ == "__main__":
    main()
    on_exit()
