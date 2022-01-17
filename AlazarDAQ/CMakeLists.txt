# Build options
set(WITH_ALAZAR_DAQ ON CACHE BOOL "Build AlazarDAQ")
set(WITH_ALAZAR_DAQ_TEST OFF CACHE BOOL "Build AlazarDAQ.test")

# Library: AlazarDAQ
if (WITH_ALAZAR_DAQ)
    include_directories("C:/AlazarTech/ATS-SDK/7.1.4/Samples_C/Include")
    add_library(AlazarDAQ AlazarDAQ.cpp AlazarDAQ.h) 
    target_link_libraries(AlazarDAQ "C:/AlazarTech/ATS-SDK/7.1.4/Samples_C/Library/x64/AtsApi.lib")
endif (WITH_ALAZAR_DAQ)

# Executable: AlazarDAQ.test
if (WITH_ALAZAR_DAQ_TEST)
    add_executable(AlazarDAQ.test test.cpp)
    target_link_libraries(AlazarDAQ.test AlazarDAQ)
    set_target_properties(AlazarDAQ.test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${PROJECT_SOURCE_DIR}/bin"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${PROJECT_SOURCE_DIR}/bin"
        RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${PROJECT_SOURCE_DIR}/bin"
        RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${PROJECT_SOURCE_DIR}/bin")
endif (WITH_ALAZAR_DAQ_TEST)