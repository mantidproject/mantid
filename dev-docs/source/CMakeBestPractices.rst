.. _CMakeBestPractices:

=================
CMake Guidelines
=================

This document outlines a few CMake guidelines which should be considered when modifying Mantid's CMake code.
A brief introduction to modern CMake can be found at https://hsf-training.github.io/hsf-training-cmake-webpage/.
The official CMake tutorial series are also very good https://cmake.org/cmake/help/latest/guide/tutorial/index.html

=============================
Creating a new CMake Library
=============================

If you are adding a new CMake library, for example to the framework, there are a couple of steps you need to take.
Firstly, lets consider a new framework target `Fitting`, which is a shared library which can be created with:

.. code-block:: cmake

   add_library(Fitting fitting.cpp fitting_1.cpp fitting.h fitting_1.h)

While not required, it is typically good practice to include the headers in the ``add_library`` call as it helps IDEs such as Visual Studio find the headers associated with the library.

This new target, ``Fitting``, will likely have a set of dependencies, these should be included using the ``target_link_libraries`` function:

.. code-block:: cmake

    target_link_libraries(<target>
                        <PRIVATE|PUBLIC|INTERFACE> <item>...
                        [<PRIVATE|PUBLIC|INTERFACE> <item>...]...)

Whether or not a dependency is PUBLIC or PRIVATE primarily depends on whether the code has been included in a public header. For example, if we had used Boost's date_time library in our header files for the Fitting target,
we would add Boost::date_time as a PUBLIC dependency of our target.

.. code-block:: cmake

    target_link_libraries(Fitting PUBLIC Boost::date_time)

Otherwise, if Boost date_time was only present in the source files we could add it as a PRIVATE dependency.

The target based linking also includes any ``INTERFACE_INCLUDE_DIRECTORIES`` of the linked target, i.e. it will naturally include header files with it. This means we do not need to use target_include_directories to include Boost's headers.

If you find yourself doing something like ``target_include_directories(Fitting PUBLIC ${Boost_include_dirs})`` you've probably done something wrong. This goes against the modern CMake target approach. The only thing target_include_directories is needed for is
to tell users of your Fitting library where it can find its headers:


.. code-block:: cmake

    target_include_directories(
                Fitting PUBLIC
                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
    )

This specifies that while building the project, the headers for fitting can be found in the inc/ subfolder.

Tips:

    - Please use this target based approach. Do not link the CMake target to variables such as ``${Boost_LIBRARIES}``.
    - Consider whether the dependencies are PUBLIC or PRIVATE. If you wrongly specify things as public it adds unnecessary dependencies to users of your library.
    - Please rethink if you are using ``target_include_directories`` to include headers from an external library, as the ``target_link_libraries`` should have already resolved those requirements.

================================
Finding a new CMake dependency
================================

If you are adding a new CMake dependency you'll need to find it using the find_package(...) CMake functions. This looks for the library using two mechanisms:

1. Uses Find(...).cmake either from the CMake inbuilt modules, or using our own ones in the buildconfig/CMake folder.
2. Uses CMake config files which are optionally installed with the library.

If neither are working, you'll likely need to create your own ``Find(...).cmake`` file. Examples of this include our own ``FindJsoncpp.cmake`` and ``FindPoco.cmake`` files. The primary steps in this file are:

1. Use find_library(MY_LIBRARY NAMES name_of_my_library) to search CMake paths for ``name_of_my_library``
2. Use find_path(MY_LIBRARY_INCLUDE_DIR header.h)
3. Finally you should create CMake targets based on these libraries using the IMPORTED specifier.

.. code-block:: cmake

    add_library(MyLibrary::mylibrary UNKNOWN IMPORTED)
    set_target_properties(
      MyLibrary::mylibrary
      PROPERTIES IMPORTED_LOCATION "${MY_LIBRARY NAMES}"
                 INTERFACE_INCLUDE_DIRECTORIES "${MY_LIBRARY_INCLUDE_DIR}"
                 IMPORTED_LINK_INTERFACE_LANGUAGES "CXX"
    )

After you've created this file, you can find mylibrary and its target ``MyLibrary::mylibrary`` using:

.. code-block:: cmake

    find_package(MyLibrary REQUIRED)


NOTE: The ``UNKNOWN`` specifier in add_library is useful on Windows as it means we aren't required to point it to the .lib export libraries. The find_library CMake function will not find these. Usage of Unknown is common place in CMake finders because of this.

=============================================
Introducing a new CMake configurable variable
=============================================

If you are introducing a new configuration variable for CMake, e.g to conditionally add a new feature, PLEASE cache the variable. If you don't cache it, you won't be able to set it on the command line. Example:

If you add a new feature and control it with a flag

.. code-block:: cmake

    set(MY_NEW_FEATURE OFF)

The only way you can change it to ON is to edit it within CMake. If you cache the variable you can set it on the command line

.. code-block:: cmake

    set(MY_NEW_FEATURE OFF CACHE BOOL "Use my new feature")

.. code-block:: sh

    cmake .. -DMY_NEW_FEATURE=ON

=============================
CMake framework exports
=============================

With the move to conda, we have created a CMake export target for the Framework libraries. If you add a new Framework library, or dependency there are a couple of things you need to consider.

1. When you add a new Framework library, alias it using the namespace Mantid:: - This means when we link to Mantid::NewTarget it can either link to our inbuilt library, or one on our system. This ensures we can have a standalone mantidqt build.

    .. code-block:: cmake

        add_library(NewTarget ${SRC_FILES} ${INC_FILES})
        add_library(Mantid::NewTarget ALIAS NewTarget)

2. Add the install commands.

    A Framework library can either be installed as a regular or plugin library.

    Regular library (ensures target is exported):

    .. code-block:: cmake

        set(TARGET_EXPORT_NAME "MantidNewTargetTargets")
        mtd_install_framework_lib(TARGETS NewTarget EXPORT_NAME ${TARGET_EXPORT_NAME})

    Plugin library:

    .. code-block:: cmake

        set(TARGET_EXPORT_NAME "MantidNewTargetTargets")
        mtd_install_framework_lib(TARGETS NewTarget PLUGIN_LIB)

    If a library is installed as a plugin library in will be installed in the plugin directory as opposed to the library directory. No headers, export files, or CMake targets are installed as plugin libraries are to be loaded dynamically
    and will not be linked against at build time.

    A library cannot be linked to a plugin library as a dependency; a plugin library is not guaranteed to be present. If this is done, errors will occur during the packaging stage.

3. Add the new target to the MODULES variable in ``MantidFrameworkConfig.cmake.in``. If it added new dependencies also add the relevant ``find_dependency`` calls.
