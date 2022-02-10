# The setuptools develop mode previously relied on .egg-link files to locate python packages in the source code
# directories. .egg-link files not supported for new setuptools versions so rely on sitecustomize.py in bin/<config>
# directory instead This code writes an additional sitecustomize.py to the site-packages directory which helps
# bin/<config>/sitecustomize.py get found It does this by adding the cwd to the path. cwd equals bin/<config> when
# running launching workbench using python <workbench shim>

# Separate to this, some content is also added to set up some paths for Windows (this code was previously part of the
# thirdparty-msvc repository) Note these paths are required by Windows at cmake configure time
if(Python_SETUPTOOLS_VERSION VERSION_GREATER_EQUAL 49.0.0)
  execute_process(
    COMMAND ${Python_EXECUTABLE} -c "from distutils.sysconfig import get_python_lib; print(get_python_lib())"
    OUTPUT_VARIABLE PYTHON_SITE_PACKAGES
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  set(SITE_CUSTOMIZE_CONTENT "")
  if(MSVC)
    set(SITE_CUSTOMIZE_CONTENT
        "${SITE_CUSTOMIZE_CONTENT}
\"\"\"
Attempts to detect if this bundle is run from a development build and updates the DLL search path
to find all thirdparty libraries. A packaged bundle has all DLLs in the main bin directory
and requires no action.

This bundle is assumed to only be used on Windows. This trick is based on the similar find_qt functionality
in PyQt5.__init__.
\"\"\"


def find_thirdparty():
    import os, sys

    # Look for a \"marker\" DLL that indicates that this looks like the layout in a user package.
    py_exe_dir = os.path.normpath(os.path.dirname(os.__file__) + '\\\\..')
    marker_dll = '\\\\zlib.dll'
    if os.path.isfile(py_exe_dir + marker_dll):
        # this looks like a package bundle where all dependent DLLs are in the same directory.
        # nothing to do.
        return

    # We assume the layout of the thirdparty directory has DLLs in bin, lib\\qt4 and lib\\qt5
    thirdparty_lib = os.path.dirname(py_exe_dir)
    thirdparty_bin = os.path.dirname(thirdparty_lib) + \"\\\\bin\"

    extra_paths = (
        py_exe_dir,  # required for C++ tests that embed Python to find Python DLL
        thirdparty_bin,
        thirdparty_bin + \"\\\\mingw\",
        thirdparty_lib + \"\\\\qt4\\\\bin\",
        thirdparty_lib + \"\\\\qt4\\\\lib\",
        thirdparty_lib + \"\\\\qt5\\\\bin\",
        thirdparty_lib + \"\\\\qt5\\\\lib\"
    )
    for dll_dir in extra_paths:
        # Add it to the search path for this process if it exists
        if os.path.isdir(dll_dir):
            os.add_dll_directory(dll_dir)
    # Update PATH as add_dll_directory does not do this
    os.environ[\"PATH\"] = \";\".join(extra_paths) + \";\" + os.environ[\"PATH\"]


find_thirdparty()
del find_thirdparty
"
    )
  endif()

  set(SITE_CUSTOMIZE_CONTENT
      "${SITE_CUSTOMIZE_CONTENT}
\"\"\"
Based on: https://code.activestate.com/recipes/552729/
Since Python 2.5 the path of the .py file being run (=cwd) seems to be added to sys.path somewhere
later after importing the file \"site\". The only case where this isn't true is running workbench
via the PyCharm pydevd script
So \"sitecustomize.py\" file is just found on the default path. Add the current working directory
to the sys.path here to give a better chance of a Mantid sitecustomize in bin/<Config> being found
Note - cwd not guaranteed to be bin/<Config> if workbench not started via \"python script.py\" so
don't do the addsitedir calls here
The mantid sitecustomize has a suffix in the name to ensure it never hides this sitecustomize.py
This code may also be run at cmake configure time in which case the sitecustomize_mantid.py may
not have been created yet so don't fail if can't import it
\"\"\"
import sys
import os
sys.path = [os.getcwd()] + sys.path
try:
    import sitecustomize_mantid
except ImportError:
    pass
"
  )

  file(WRITE ${PYTHON_SITE_PACKAGES}/sitecustomize.py "${SITE_CUSTOMIZE_CONTENT}")
endif()
