cmake_minimum_required(VERSION 2.8)
cmake_policy(SET CMP0020 NEW)
if (POLICY CMP0043)
	cmake_policy(SET CMP0043 NEW)
endif (POLICY CMP0043)

project(Erasmus)

# Project header directory
include_directories("${PROJECT_SOURCE_DIR}" "${PROJECT_BINARY_DIR}")
link_directories(${PROJECT_SOURCE_DIR})

# Compiler options: surpress warnings
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_SCL_SECURE_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS /wd4819")

# ----------

# Qt
set(CMAKE_PREFIX_PATH "${PROJECT_SOURCE_DIR}/lib/qt-msvc2013_64-5.4.0")
find_package(Qt5Core REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5AxBase REQUIRED)
find_package(Qt5AxContainer REQUIRED)
set(CMAKE_AUTOMOC ON)

# OpenCV
set(CMAKE_PREFIX_PATH "${PROJECT_SOURCE_DIR}/lib/opencv")
set(OpenCV_STATIC OFF)
find_package(OpenCV REQUIRED)

# Intel IPP
include_directories("$ENV{ProgramFiles}/Intel/Composer XE/ipp/include")
link_directories("$ENV{ProgramFiles}/Intel/Composer XE/ipp/lib/intel64")
link_directories("$ENV{ProgramFiles}/Intel/Composer XE/compiler/lib/intel64")

# numcpp
include_directories(numcpp)

# ----------

# Library: OFDIProcess3
set(CMAKE_MFC_FLAG 2)
add_library(OFDIProcess3
	OFDIProcess3/AlazarProcess.cpp
	OFDIProcess3/AlazarProcess.h
	OFDIProcess3/ImageProcess.cpp
	OFDIProcess3/ImageProcess.h
	OFDIProcess3/OFDI.cpp
	OFDIProcess3/OFDI.h
	OFDIProcess3/IntensityProcess.cpp
	OFDIProcess3/IntensityProcess.h)

qt5_use_modules(OFDIProcess3 Core Widgets AxBase AxContainer)
target_link_libraries(OFDIProcess3 ippcoremt ipps ippi ippac ${OpenCV_LIBS})

# Executable: OFDIProcess3.test
add_executable(OFDIProcess3.test
	OFDIProcess3/test.cpp)

target_link_libraries(OFDIProcess3.test OFDIProcess3)

set_target_properties(OFDIProcess3.test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${PROJECT_SOURCE_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${PROJECT_SOURCE_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${PROJECT_SOURCE_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${PROJECT_SOURCE_DIR}/bin")

# Library: DataAcquisition
add_library(DataAcquisition
    DataAcquisition/DataRecording.h
    DataAcquisition/DataRecording.cpp
	DataAcquisition/DataBufferModel.h
	DataAcquisition/DataBufferModel.cpp
	DataAcquisition/Calibration.h
	DataAcquisition/Calibration.cpp
	DataAcquisition/MemoryManager.cpp
	DataAcquisition/MemoryManager.h
	DataAcquisition/ZabberStage.cpp
	DataAcquisition/ZabberStage.h
	DataAcquisition/pserial.c
	DataAcquisition/pserial.h
	DataAcquisition/CameraCapture.cpp
	DataAcquisition/CameraCapture.h

	DataAcquisition/RecordResultView.cpp
	DataAcquisition/RecordResultView.h
	DataAcquisition/ConfigurationFile.cpp
	DataAcquisition/ConfigurationFile.h)

qt5_use_modules(DataAcquisition Core Widgets)

# subdirectories
add_subdirectory(AlazarDAQ)
add_subdirectory(SignatecDAQ)
add_subdirectory(NIDAQ)
add_subdirectory(Erasmus)

# ----- 

# Target for copying dll files to executable dir
add_custom_target(COPY_DLL_FILES)

# Copy IPP dll files
add_custom_command(TARGET COPY_DLL_FILES PRE_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
	"$ENV{ProgramFiles}/Intel/Composer XE/redist/intel64/ipp"
	${PROJECT_SOURCE_DIR}/bin)	

# Copy OpenCV dll files
set(OPENCV_DLLS
    ${PROJECT_SOURCE_DIR}/lib/opencv/x64/vc12/bin/opencv_core2411.dll
    ${PROJECT_SOURCE_DIR}/lib/opencv/x64/vc12/bin/opencv_core2411d.dll
    ${PROJECT_SOURCE_DIR}/lib/opencv/x64/vc12/bin/opencv_highgui2411.dll
    ${PROJECT_SOURCE_DIR}/lib/opencv/x64/vc12/bin/opencv_highgui2411d.dll
    ${PROJECT_SOURCE_DIR}/lib/opencv/x64/vc12/bin/opencv_imgproc2411.dll
    ${PROJECT_SOURCE_DIR}/lib/opencv/x64/vc12/bin/opencv_imgproc2411d.dll)

foreach(file_i ${OPENCV_DLLS})
    add_custom_command(
        TARGET COPY_DLL_FILES PRE_BUILD
        COMMAND ${CMAKE_COMMAND}
        ARGS -E copy ${file_i} ${PROJECT_SOURCE_DIR}/bin)
endforeach(file_i)

# Copy Qt dll files
set(QT_DLLS
	${PROJECT_SOURCE_DIR}/lib/qt-msvc2013_64-5.4.0/bin/icudt53.dll
	${PROJECT_SOURCE_DIR}/lib/qt-msvc2013_64-5.4.0/bin/icuin53.dll
	${PROJECT_SOURCE_DIR}/lib/qt-msvc2013_64-5.4.0/bin/icuuc53.dll
	${PROJECT_SOURCE_DIR}/lib/qt-msvc2013_64-5.4.0/bin/libEGL.dll
	${PROJECT_SOURCE_DIR}/lib/qt-msvc2013_64-5.4.0/bin/libEGLd.dll
	${PROJECT_SOURCE_DIR}/lib/qt-msvc2013_64-5.4.0/bin/libGLESv2.dll
	${PROJECT_SOURCE_DIR}/lib/qt-msvc2013_64-5.4.0/bin/libGLESv2d.dll
	${PROJECT_SOURCE_DIR}/lib/qt-msvc2013_64-5.4.0/bin/Qt5Core.dll
	${PROJECT_SOURCE_DIR}/lib/qt-msvc2013_64-5.4.0/bin/Qt5Cored.dll
	${PROJECT_SOURCE_DIR}/lib/qt-msvc2013_64-5.4.0/bin/Qt5Gui.dll
	${PROJECT_SOURCE_DIR}/lib/qt-msvc2013_64-5.4.0/bin/Qt5Guid.dll
	${PROJECT_SOURCE_DIR}/lib/qt-msvc2013_64-5.4.0/bin/Qt5Widgets.dll
	${PROJECT_SOURCE_DIR}/lib/qt-msvc2013_64-5.4.0/bin/Qt5Widgetsd.dll)

foreach(file_i ${QT_DLLS})
    add_custom_command(
    TARGET COPY_DLL_FILES PRE_BUILD
    COMMAND ${CMAKE_COMMAND}
    ARGS -E copy ${file_i} ${PROJECT_SOURCE_DIR}/bin)
endforeach(file_i)

add_custom_command(TARGET COPY_DLL_FILES PRE_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
	${PROJECT_SOURCE_DIR}/lib/qt-msvc2013_64-5.4.0/plugins/platforms
	${PROJECT_SOURCE_DIR}/bin/plugins/platforms)
