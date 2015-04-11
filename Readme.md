AMD-Perf-Lib
============

Author: Matthäus G. Chajdas

Overview
--------

AMD provides a C library to query the hardware performance counters of their GPUs, called **GPUPerfAPI**. This project provides an easy-to-use, C++ based wrapper on top of the C API.

Getting started
---------------

The project requires Visual Studio 2012 or later, or GCC 4.8/Clang 3.4. Most likely, older GCC/Clang versions will work as well, but haven't been tested. When using GCC/Clang, please enable ``-std=c++11``.

There is a basic ``CMakeLists.txt`` file to help you get started. You have to define the CMake variables ``AMD_PERF_LIBRARY_OS_WINDOWS`` or ``AMD_PERF_LIBRARY_OS_LINUX`` while configuring. Alternatively, you can simply compile ``PerfLib.cpp`` and ``#define`` ``AMD_PERF_API_WINDOWS=1`` or ``AMD_PERF_API_LINUX=1`` using the C++ preprocessor.

It has been tested on Windows 7, with a HD 7970; on Windows 8.1 with a R9 290X and should also work on Linux.

Notes
-----

This project is developed as part of a bigger project; this repository contains only code drops from the main development repository. Don't be surprised if sometimes multiple changes are committed together!

License
-------

This project is licensed under the 2-clause BSD. See ``LICENSE.txt`` for details.
