# Copyright (C) 2006-2018 Istituto Italiano di Tecnologia (IIT)
# Copyright (C) 2006-2010 RobotCub Consortium
# All rights reserved.
#
# This software may be modified and distributed under the terms of the
# BSD-3-Clause license. See the accompanying LICENSE file for details.

include(CMakeDependentOption)
cmake_dependent_option(CREATE_YARPC "Compile experimental stable C library" FALSE
                       YARP_COMPILE_UNMAINTAINED FALSE)
mark_as_advanced(CREATE_YARPC)

cmake_dependent_option(CREATE_YARPC_STANDALONE "Make yarpc library fully standalone" TRUE
                       CREATE_YARPC FALSE)
mark_as_advanced(CREATE_YARPC_STANDALONE)

if(CREATE_YARPC)
  set(CMAKE_INCLUDE_CURRENT_DIR TRUE)

  unset(YARP_COPY_SRC)
  if(CREATE_YARPC_STANDALONE)
    include_directories(SYSTEM ${ACE_INCLUDE_DIRS})
    # in some cases, want to be fully stand-alone to not depend on C++ ABI.
    foreach(_comp OS sig dev init)
      get_property(${_comp}_src TARGET YARP_${_comp} PROPERTY SOURCES)
      foreach(_file ${${_comp}_src})
        list(APPEND YARP_COPY_SRC ${CMAKE_SOURCE_DIR}/src/libYARP_${_comp}/${_file})
        set_property(SOURCE ${CMAKE_SOURCE_DIR}/src/libYARP_${_comp}/${_file} APPEND_STRING PROPERTY COMPILE_FLAGS " -fvisibility=hidden")
      endforeach()
      get_property(YARP_${_comp}_INCLUDE_DIRS TARGET YARP_${_comp} PROPERTY INCLUDE_DIRECTORIES)
      include_directories(${YARP_${_comp}_INCLUDE_DIRS})
    endforeach()
  endif(CREATE_YARPC_STANDALONE)

  set(yarpc_HDRS yarp.h
                 yarpimpl.h)

  set(yarpc_SRCS yarp.cpp
                 yarpnetwork.cpp
                 yarpport.cpp
                 yarpcontact.cpp
                 yarpreader.cpp
                 yarpwriter.cpp
                 yarpportable.cpp
                 yarpstring.cpp
                 yarpsemaphore.cpp
                 yarpthread.cpp
                 yarptime.cpp
                 yarpbottle.cpp)

  foreach(_file ${yarpc_SRCS} ${yarpc_HDRS})
    set_property(SOURCE ${_file} APPEND_STRING PROPERTY COMPILE_FLAGS " -fvisibility=default")
  endforeach()

  add_library(yarpc ${yarpc_HDRS}
                    ${yarpc_SRCS}
                    ${YARP_COPY_SRC})
  add_library(YARP::yarpc ALIAS yarpc)

  target_link_libraries(yarpc PRIVATE ${ACE_LIBRARIES})

  if(NOT CREATE_YARPC_STANDALONE)
    target_link_libraries(yarpc PRIVATE YARP::YARP_OS
                                        YARP::YARP_init)
    if(YARP_LINK_PLUGINS)
      if(YARP_COMPILE_CARRIER_PLUGINS)
        set_property(SOURCE ${init_src}
                     APPEND PROPERTY COMPILE_DEFINITIONS
                     PLUGIN_INIT_FUNCTION=add_yarpcar_plugins)
        target_link_libraries(yarpc PRIVATE YARP::yarpcar)
      endif()

      if(YARP_COMPILE_DEVICE_PLUGINS)
        set_property(SOURCE ${init_src}
                     APPEND PROPERTY COMPILE_DEFINITIONS
                     PLUGIN_INIT_FUNCTION2=add_yarpmod_plugins)
          target_link_libraries(yarpc PRIVATE YARP::yarpmod)
      endif()
    endif()

  else()
    if(LINUX)
      target_link_libraries(yarpc PRIVATE pthread m dl rt)
    endif()
    if(MINGW)
      target_link_libraries(yarpc PRIVATE stdc++)
      target_link_libraries(yarpc PRIVATE m ws2_32 mswsock netapi32 winmm)
      set_target_properties(yarpc PROPERTIES LINK_FLAGS "-Wl,--output-def,${LIBRARY_OUTPUT_PATH}/libyarpc.def")
    endif()
  endif(NOT CREATE_YARPC_STANDALONE)

  set_property(TARGET yarpc PROPERTY FOLDER "Unmaintained")

  if(YARP_COMPILE_TESTS)
    add_executable(yarpc_test1 test1.cpp)
    target_link_libraries(yarpc_test1 YARP::yarpc)

    add_executable(yarpc_test2 test2.cpp)
    target_link_libraries(yarpc_test2 YARP::yarpc)
  endif(YARP_COMPILE_TESTS)

endif(CREATE_YARPC)
