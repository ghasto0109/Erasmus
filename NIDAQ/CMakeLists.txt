# Build options
set(WITH_NI_DAQ OFF CACHE BOOL "Build NI DAQ")
set(WITH_NI_DAQ_TEST OFF CACHE BOOL "Build NI DAQ test")

# Library: NI_DAQ
if (WITH_NI_DAQ)
    add_library(NI_DAQ 
        AnalogOutput.h
        AnalogOutput.cpp
        AnalogOutput2.h
        AnalogOutput2.cpp)

    target_link_libraries(NI_DAQ ${CMAKE_CURRENT_SOURCE_DIR}/NIDAQmx.lib)
endif (WITH_NI_DAQ)

# Executable: NI_DAQ.test
if (WITH_NI_DAQ_TEST)
    add_executable(NI_DAQ.test test.cpp)
    target_link_libraries(NI_DAQ.test NI_DAQ)
    set_target_properties(NI_DAQ.test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${PROJECT_SOURCE_DIR}/bin"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${PROJECT_SOURCE_DIR}/bin"
        RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${PROJECT_SOURCE_DIR}/bin"
        RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${PROJECT_SOURCE_DIR}/bin")
endif (WITH_NI_DAQ_TEST)