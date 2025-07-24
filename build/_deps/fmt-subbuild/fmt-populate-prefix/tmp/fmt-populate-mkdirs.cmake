# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/root/deribit-trading-system/build/_deps/fmt-src"
  "/root/deribit-trading-system/build/_deps/fmt-build"
  "/root/deribit-trading-system/build/_deps/fmt-subbuild/fmt-populate-prefix"
  "/root/deribit-trading-system/build/_deps/fmt-subbuild/fmt-populate-prefix/tmp"
  "/root/deribit-trading-system/build/_deps/fmt-subbuild/fmt-populate-prefix/src/fmt-populate-stamp"
  "/root/deribit-trading-system/build/_deps/fmt-subbuild/fmt-populate-prefix/src"
  "/root/deribit-trading-system/build/_deps/fmt-subbuild/fmt-populate-prefix/src/fmt-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/root/deribit-trading-system/build/_deps/fmt-subbuild/fmt-populate-prefix/src/fmt-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/root/deribit-trading-system/build/_deps/fmt-subbuild/fmt-populate-prefix/src/fmt-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
