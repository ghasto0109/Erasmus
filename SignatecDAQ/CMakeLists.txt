# Build options
set(WITH_SIGNATEC_DAQ ON CACHE BOOL "Build SignatecDAQ")
set(WITH_SIGNATEC_DAQ_TEST OFF CACHE BOOL "Build SignatecDAQ.test")

# Library: SignatecDAQ
if (WITH_SIGNATEC_DAQ)
    include_directories("$ENV{ProgramW6432}/Signatec/PX14400/Include")

    add_library(SignatecDAQ SignatecDAQ.cpp SignatecDAQ.h) 
    target_link_libraries(SignatecDAQ $ENV{ProgramW6432}/Signatec/PX14400/Lib/PX14_64.lib)
endif (WITH_SIGNATEC_DAQ)

# Executable: SignatecDAQ.test
if (WITH_SIGNATEC_DAQ_TEST)
    add_executable(SignatecDAQ.test test.cpp testFileSave.cpp)
    target_link_libraries(SignatecDAQ.test SignatecDAQ)
    set_target_properties(SignatecDAQ.test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${PROJECT_SOURCE_DIR}/bin"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${PROJECT_SOURCE_DIR}/bin"
        RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${PROJECT_SOURCE_DIR}/bin"
        RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${PROJECT_SOURCE_DIR}/bin")
endif (WITH_SIGNATEC_DAQ_TEST)