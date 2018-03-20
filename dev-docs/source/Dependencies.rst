============
Dependencies
============

The large external libraries that we use in Mantid (boost, Poco & Qt)
have overlapping functionality in certain areas. In this situation we
want to be consistent in only using one\ :sup:`\*` library for a
particular piece of functionality. The table below sets out what we use
in various areas.

+-------------------------+-------+-------+----+-------------+
| Functionality           | Boost | Poco  | Qt | Other       |
+=========================+=======+=======+====+=============+
| File / directories      |       | **X** |    |             |
+-------------------------+-------+-------+----+-------------+
| Notifications / signals |       | **X** |    |             |
+-------------------------+-------+-------+----+-------------+
| Threads / threadpools   |       | **X** | X  |             |
+-------------------------+-------+-------+----+-------------+
| Regular expressions     | **X** |       |    |             |
+-------------------------+-------+-------+----+-------------+
| DateTime                | **X** |       |    |             |
+-------------------------+-------+-------+----+-------------+
| Setting & Properties    |       | **X** | X  |             |
+-------------------------+-------+-------+----+-------------+
| Logging                 |       | **X** |    |             |
+-------------------------+-------+-------+----+-------------+
| XML                     |       | X     | ?X |             |
+-------------------------+-------+-------+----+-------------+
| Triangulation           |       |       |    | OpenCascade |
+-------------------------+-------+-------+----+-------------+
| Python                  | X     |       |    | SIP         |
+-------------------------+-------+-------+----+-------------+

:sup:`\*` In some (rare) cases we may want to use Qt for something up at
the graphical layer, and something different in the framework where Qt
is not available.

Also noteworthy is the `Compiler
Support <http://en.cppreference.com/w/cpp/compiler_support>`__ for
various "advanced" language features.
