# Build options
set(WITH_ERASMUS ON CACHE BOOL "Build Erasmus")
set(WITH_ERASMUS_ALAZAR OFF CACHE BOOL "Build Erasmus.Alazar")
set(WITH_ERASMUS_DOPPLER OFF CACHE BOOL "Build Erasmus.Doppler")
set(WITH_ERASMUS_RODENT OFF CACHE BOOL "Build Erasmus.Rodent")

set(OFDI_PROCESS_VIEWER_FILES
    OFDIProcessViewer.h
    OFDIProcessViewer.cpp
    QPictureBox.h
    QPictureBox.cpp
    QScopeControl.h
    QScopeControl.cpp
    IScopePX14.h)

# Executable: Erasmus
if (WITH_ERASMUS)
    include_directories("${CMAKE_CURRENT_BINARY_DIR}")
    qt5_wrap_ui(MAIN_WINDOW_UI MainWindow.ui)

    add_executable(Erasmus 
        MainWindow.h
        MainWindow.cpp
        ${OFDI_PROCESS_VIEWER_FILES})

    qt5_use_modules(Erasmus Core Widgets)
    target_link_libraries(Erasmus DataAcquisition SignatecDAQ OFDIProcess3)
    set_target_properties(Erasmus PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${PROJECT_SOURCE_DIR}/bin"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${PROJECT_SOURCE_DIR}/bin"
        RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${PROJECT_SOURCE_DIR}/bin"
        RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${PROJECT_SOURCE_DIR}/bin")

endif (WITH_ERASMUS)

# Executable: Erasmus.Alazar
if (WITH_ERASMUS_ALAZAR)
    include_directories("${CMAKE_CURRENT_BINARY_DIR}")
    qt5_wrap_ui(MAIN_WINDOW_UI MainWindow.ui)

    add_executable(Erasmus.Alazar
        MainWindow.Alazar.h
        MainWindow.Alazar.cpp
        ${OFDI_PROCESS_VIEWER_FILES}
        ${MAIN_WINDOW_UI})

    qt5_use_modules(Erasmus.Alazar Core Widgets)
    target_link_libraries(Erasmus.Alazar DataAcquisition AlazarDAQ OFDIProcess3 NI_DAQ)
    set_target_properties(Erasmus.Alazar PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${PROJECT_SOURCE_DIR}/bin"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${PROJECT_SOURCE_DIR}/bin"
        RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${PROJECT_SOURCE_DIR}/bin"
        RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${PROJECT_SOURCE_DIR}/bin")

endif (WITH_ERASMUS_ALAZAR)

# Executable: Erasmus.Doppler
if (WITH_ERASMUS_DOPPLER)
    include_directories("${CMAKE_CURRENT_BINARY_DIR}")
    qt5_wrap_ui(MAIN_WINDOW_UI MainWindow.ui)

    add_executable(Erasmus.Doppler 
        MainWindow.Doppler.h
        MainWindow.Doppler.cpp
        ${OFDI_PROCESS_VIEWER_FILES}
        ${MAIN_WINDOW_UI}
        Erasmus.cpp)

    qt5_use_modules(Erasmus.Doppler Core Widgets)
    target_link_libraries(Erasmus.Doppler DataAcquisition SignatecDAQ OFDIProcess3 NI_DAQ)
    set_target_properties(Erasmus.Doppler PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${PROJECT_SOURCE_DIR}/bin"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${PROJECT_SOURCE_DIR}/bin"
        RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${PROJECT_SOURCE_DIR}/bin"
        RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${PROJECT_SOURCE_DIR}/bin")

endif (WITH_ERASMUS_DOPPLER)

# Executable: Erasmus.Rodent
if (WITH_ERASMUS_RODENT)
    include_directories("${CMAKE_CURRENT_BINARY_DIR}")
    qt5_wrap_ui(MAIN_WINDOW_UI MainWindow.ui)

    add_executable(Erasmus.Rodent 
        MainWindow.Rodent.h
        MainWindow.Rodent.cpp
        ${OFDI_PROCESS_VIEWER_FILES}
        ${MAIN_WINDOW_UI})

    qt5_use_modules(Erasmus.Rodent Core Widgets)
    target_link_libraries(Erasmus.Rodent DataAcquisition SignatecDAQ OFDIProcess3 NI_DAQ)
    set_target_properties(Erasmus.Rodent PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${PROJECT_SOURCE_DIR}/bin"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${PROJECT_SOURCE_DIR}/bin"
        RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${PROJECT_SOURCE_DIR}/bin"
        RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${PROJECT_SOURCE_DIR}/bin")

endif (WITH_ERASMUS_RODENT)
