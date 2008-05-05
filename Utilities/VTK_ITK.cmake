
SET(SAVED_SHARED_LIBS_STATE ${BUILD_SHARED_LIBS})

SET(BUILD_SHARED_LIBS ON)

# these settings are for ITK/VTK only. Maybe this file has to be moved
# to a subdirectory to prevent collisions with other util libraries if they use
# BUILD_TESTING and BUILD_EXAMPLES as well.
SET(BUILD_TESTING OFF)
SET(BUILD_EXAMPLES OFF)

# these are optional 
SET(MITK_ITK_SOURCE_DIR "" CACHE PATH "An ITK source tree to be used for integrated build with MITK")
SET(MITK_VTK_SOURCE_DIR "" CACHE PATH "An VTK source tree to be used for integrated build with MITK")
MARK_AS_ADVANCED(MITK_ITK_SOURCE_DIR)
MARK_AS_ADVANCED(MITK_VTK_SOURCE_DIR)
IF(EXISTS "${MITK_ITK_SOURCE_DIR}")
  ADD_SUBDIRECTORY("${MITK_ITK_SOURCE_DIR}" itk-bin)
  SET(ITK_DIR "${CMAKE_CURRENT_BINARY_DIR}/itk-bin" CACHE PATH "ITK for MITK binary dir")
  SET(ITK_DIR "${CMAKE_CURRENT_BINARY_DIR}/itk-bin")
ENDIF(EXISTS "${MITK_ITK_SOURCE_DIR}")

IF(EXISTS "${MITK_VTK_SOURCE_DIR}")
  SET(VTK_USE_PARALLEL ON CACHE BOOL "")
  SET(VTK_USE_GUISUPPORT ON CACHE BOOL "")
  SET(VTK_USE_QVTK ON CACHE BOOL "")
  ADD_SUBDIRECTORY("${MITK_VTK_SOURCE_DIR}" vtk-bin)
  SET(VTK_DIR "${CMAKE_CURRENT_BINARY_DIR}/vtk-bin" CACHE PATH "VTK for MITK binary dir")
  SET(VTK_DIR "${CMAKE_CURRENT_BINARY_DIR}/vtk-bin")
ENDIF(EXISTS "${MITK_VTK_SOURCE_DIR}")

SET(BUILD_SHARED_LIBS ${SAVED_SHARED_LIBS_STATE})

