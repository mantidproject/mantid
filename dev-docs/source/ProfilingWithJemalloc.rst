
Profiling with ``jemalloc``
===========================

The ``jemalloc`` allocator is a drop-in replacement for ``malloc``.  It is used primarily as shared library preloaded via the dynamic linker, although it can also be linked statically.  ``jemalloc`` offers substantial improvement over ``malloc`` in terms of its concurrency support, and in regards to its avoidance of memory fragmentation.  When Mantid is launched via ``workbench``, or when a python interpreter is launched to call ``mantid`` algorithms from scripts (in the recommended manner), the ``jemalloc`` library will be preloaded automatically.

When used as an allocator, ``jemalloc`` provides extensive allocation  control and profiling hooks.  These hooks may be accessed either using the environment variable ``MALLOC_CONF`` (and several other environment variables) or also programatically using the *nonstandard* ``mallctl`` entrypoint.  Note here that there are *very many* control hooks available!  In this note, only a few specific profiling techniques will be elaborated, primarily using specific examples.

It should *always* be kept in mind that although Mantid does use ``jemalloc``, its usage is only partially optimized, and there is always room for improvement.  This means that the first solution that should be considered during profiling is the possibility of further optimizing ``jemalloc``'s control settings during Mantid's *loading* process.

**All of the following examples assume a standard developer environment on linux machine.**  Most other OS should also work, and the setup commands will be similar, but this note won't cover those details.


Build ``jemalloc`` including profiling
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The standard ``jemalloc`` library does not include the profiling hooks, and it will be necessary to download the source code, and build a special version of the library, in order to use them.

``jemalloc``'s github repository (now archived) is at: https://github.com/jemalloc/jemalloc.  Be sure to download the source code corresponding to the release of ``jemalloc`` that Mantid is currently using (now 5.2.0).

``jemalloc`` uses the ``autoconf`` system.  To build from the root of the ``jemalloc`` source tree::

    ./autogen.sh
    ./configure --enable-prof --enable-shared --prefix=<your installation prefix>
    make
    make install


Use ``jemalloc`` to profile an existing Mantid build
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Any existing build of Mantid can be successfully profiled simply by changing the ``LD_PRELOAD`` to include the new ``jemalloc`` build instead of the standard one.  This example will also show how to enable profiling using the ``MALLOC_CONF`` environment variable.  Here's how to generate a series of memory-allocation profiles, with a new profile written every time memory allocation increases by 1GB::

    # Notes: enable and activate profiling, generate a profile every ~1GB of additional memory allocated.
    #   The profile-dump interval is expressed somewhat cryptically as $\log_2$\ of the memory quantity:
    #   here 1GB == 30 bits.
    export MALLOC_CONF="prof:true,prof_active:true,lg_prof_interval:30,prof_prefix:jeprof.out"

    LOCAL_PRELOAD=<your installation prefix>/lib/libjemalloc.so.2

    # To get your `PYTHONPATH`, you can enter `import sys; ';'.join(sys.path)` in the `IPython` window in workbench,
    #   although for a working installation, you probably don't need to worry about it.
    #   First try removing the `PYTHONPATH` lines below and just assume that your Pixi environment has set the path correctly!
    LOCAL_PYTHONPATH=<Mantid's PYTHONPATH>

    LD_PRELOAD=${LOCAL_PRELOAD} \
        PYTHONPATH=${LOCAL_PYTHONPATH} \
        ${CONDA_PREFIX}/bin/python -m workbench


Optimize the Mantid build for profiling using ``jemalloc``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

There are several flags that can be set to *optimize* a Mantid build for ``jemalloc`` profiling.  Mostly the objectives are to make sure that all of the line-number and stack-frame information is included in the build::

    # From the Mantid repository root:
    mkdir build; cd build
    cmake .. -GNinja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_C_FLAGS_RELWITHDEBINFO="-O2 -g -DNDEBUG -fno-omit-frame-pointer" -DCMAKE_CXX_FLAGS_RELWITHDEBINFO="-O2 -g -DNDEBUG -fno-omit-frame-pointer"
    cmake --build . --target AllTests


Viewing and analyzing memory-allocation profiles
================================================

**The next examples assume that you have already generated a profile using MALLOC_CONF, or by modifying the code and running a script, as discussed above.**

Any of the examples shown next to *display* profiles can also be used to display the *difference* between any two profiles.  See the ``jemalloc`` man page at https://jemalloc.net/jemalloc.3.html, or its wiki at  https://github.com/jemalloc/jemalloc/wiki.


Memory-allocation details: create an overview graph
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

``jeprof`` can generate SVG, and several other formats of, graphs showing all of the allocation details::

    export JEPROF='<your installation prefix>/bin/jeprof'

    # The basic form of the command:
    ${JEPROF} --svg /path/to/program /path/to/dump.heap > heap.svg

    # A Mantid-specific example, assuming that you've been running a script in Python:
    ${JEPROF} --svg ${CONDA_PREFIX}/bin/python /path/to/dump.heap > heap.svg

Now you can view the ``svg`` in your browser (``firefox`` is useful for this, but ``google-chrome`` will also be OK)::

    firefox heap.svg &


Memory-allocation details, by source-line number
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

We can get a by-line-number allocation profile of any function (, including even inline functions), from any suitable profile.  (It's not necessary to generate a profile for each function you're interested in!)::

    export JEPROF=<your installation prefix>/bin/jeprof

    # Assume that the profile you want to examine has already generated, and is contained in the file <your profile>.dat
    ${JEPROF} --lines --inuse_space --list="GridDetector::createLayer" ${CONDA_PREFIX}/bin/python <your profile>.dat > <your profile>.heap.txt


The text output will look something like this::

    ROUTINE ====================== Mantid::Geometry::GridDetector::createLayer in /mnt/R5_data1/data1/workspaces/ORNL-work/mantid/Framework/Geometry/src/Instrument/GridDetector.cpp
     258.6  322.1 Total MB (flat / cumulative)
         .      .  432:     return m_gridBase->getRelativePosAtXYZ(x, y, z) * V3D(scalex, scaley, scalez);
         .      .  433:   } else
         .      .  434:     return V3D(m_xstart + m_xstep * x, m_ystart + m_ystep * y, m_zstart + m_zstep * z);
         .      .  435: }
         .      .  436:
    ---
         .      .  437: void GridDetector::createLayer(const std::string &name, CompAssembly *parent, int iz, int &minDetID, int &maxDetID) {
         .      .  438:   // Loop and create all detectors in this layer.
        ~SNIP~
         .      .  469:       // Create the detector from the given id & shape and with xColumn as the
         .      .  470:       // parent.
     258.6  314.1  471:       auto *detector = new GridDetectorPixel(oss.str(), id, m_shape, xColumn, this, size_t(ix), size_t(iy), size_t(iz));
         .      .  472:
        ~SNIP~

This specific example shows the *huge* allocation associated with a grid detector expansion during an ``EventWorkspace`` ``instrument`` initialization.  (Note that each detector pixel [out of possibly millions] is initialized using its own *name*, as a string!)


More advanced techniques: in-depth profiling of algorithm steps
===============================================================

The most direct way to profile the allocation details associated with step-wise execution of any algorithm is to use the ``jemalloc`` ``mallctl`` entry point.  We still need to use ``MALLOC_CONF`` to enable profiling::

    export MALLOC_CONF="prof:true"


For this example, we modify a ``C++``-source file and add the following section (near the top)::


    // =============================================================
    #include <chrono>
    #include <cstring>
    #include <string>
    #include <thread>
    #include <jemalloc/jemalloc.h>
    namespace {
      bool mem_stats(const std::string& path) {
        const char *path_ = path.c_str();
        int err = mallctl("prof.dump",
                          nullptr, nullptr,
                          const_cast<char**>(&path_),
                          sizeof(const char*));
        return err == 0;
      }

      void periodic_mem_stats(std::chrono::seconds period, const std::string& path) {
        for (;;) {
          mem_stats(path);
          std::this_thread::sleep_for(period);
        }
      }
    }
    const std::string STATS_ROOT = "<your profiles dump directory path>";
    // =============================================================


Now, whenever we want to generate a new profile after the execution of a section of code, we simply call the function::

    mem_stats(STATS_ROOT + "/loadEvents-exit.dat");


After building Mantid, setting ``MALLOC_CONF`` as above, and executing an appropriate Python script: this generates a profile and writes it to the on-disk location ``"${STATS_ROOT}/loadEvents-exit.dat"``.
