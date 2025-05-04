CMAKE_MINIMUM_REQUIRED(VERSION 3.10)

# Name of the project
PROJECT(TargetPractice)

# Use glob to get the list of all source files.
# We don't really need to include header and resource files to build, but it's
# nice to have them also show up in IDEs.
IF(${SOL})
  FILE(GLOB_RECURSE SOURCES "src0/*.cpp")
  FILE(GLOB_RECURSE HEADERS "src0/*.h")
  FILE(GLOB_RECURSE GLSL "resources0/*.glsl")
ELSE()
  FILE(GLOB_RECURSE SOURCES "src/*.cpp")
  FILE(GLOB_RECURSE HEADERS "src/*.h")
  FILE(GLOB_RECURSE GLSL "resources/*.glsl")
ENDIF()

# Generate compile_commands.json, used by Clangd, Clion, etc.
SET(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set the executable.
ADD_EXECUTABLE(${CMAKE_PROJECT_NAME} ${SOURCES} ${HEADERS} ${GLSL})

# Get the GLM environment variable. Since GLM is a header-only library, we
# just need to add it to the include directory.
SET(GLM_INCLUDE_DIR "$ENV{GLM_INCLUDE_DIR}")
IF(NOT GLM_INCLUDE_DIR)
  # The environment variable was not set
  SET(ERR_MSG "Please point the environment variable GLM_INCLUDE_DIR to the root directory of your GLM installation.")
  IF(WIN32)
    # On Windows, try the default location
    MESSAGE(STATUS "Looking for GLM in ${DEF_DIR_GLM}")
    IF(IS_DIRECTORY ${DEF_DIR_GLM})
      MESSAGE(STATUS "Found!")
      SET(GLM_INCLUDE_DIR ${DEF_DIR_GLM})
    ELSE()
      MESSAGE(FATAL_ERROR ${ERR_MSG})
    ENDIF()
  ELSE()
    MESSAGE(FATAL_ERROR ${ERR_MSG})
  ENDIF()
ENDIF()
INCLUDE_DIRECTORIES(${GLM_INCLUDE_DIR})

# Get the GLFW environment variable. There should be a CMakeLists.txt in the
# specified directory.
SET(GLFW_DIR "$ENV{GLFW_DIR}")
IF(NOT GLFW_DIR)
  # The environment variable was not set
  SET(ERR_MSG "Please point the environment variable GLFW_DIR to the root directory of your GLFW installation.")
  IF(WIN32)
    # On Windows, try the default location
    MESSAGE(STATUS "Looking for GLFW in ${DEF_DIR_GLFW}")
    IF(IS_DIRECTORY ${DEF_DIR_GLFW})
      MESSAGE(STATUS "Found!")
      SET(GLFW_DIR ${DEF_DIR_GLFW})
    ELSE()
      MESSAGE(FATAL_ERROR ${ERR_MSG})
    ENDIF()
  ELSE()
    MESSAGE(FATAL_ERROR ${ERR_MSG})
  ENDIF()
ENDIF()
OPTION(GLFW_BUILD_EXAMPLES "GLFW_BUILD_EXAMPLES" OFF)
OPTION(GLFW_BUILD_TESTS "GLFW_BUILD_TESTS" OFF)
OPTION(GLFW_BUILD_DOCS "GLFW_BUILD_DOCS" OFF)
IF(CMAKE_BUILD_TYPE MATCHES Release)
  ADD_SUBDIRECTORY(${GLFW_DIR} ${GLFW_DIR}/release)
ELSE()
  ADD_SUBDIRECTORY(${GLFW_DIR} ${GLFW_DIR}/debug)
ENDIF()
INCLUDE_DIRECTORIES(${GLFW_DIR}/include)
TARGET_LINK_LIBRARIES(${CMAKE_PROJECT_NAME} glfw ${GLFW_LIBRARIES})

find_library(FLAC_LIBRARY NAMES FLAC)

if(NOT FLAC_LIBRARY)
  message(WARNING "System FLAC not found, falling back to /opt/flac-1.3.2")

  link_directories(/opt/flac-1.3.2/lib)
  include_directories(/opt/flac-1.3.2/include)

  # Try finding FLAC again
  find_library(FLAC_LIBRARY NAMES FLAC PATHS /opt/flac-1.3.2/lib)
endif()

if(NOT FLAC_LIBRARY)
  message(FATAL_ERROR "FLAC library not found, even after trying fallback!")
endif()

# Set up SFML library
SET(SFML3_DIR "$ENV{SFML3_DIR}")
SET(SFML_DIR ${SFML3_DIR})
FIND_PACKAGE(SFML 3 COMPONENTS Audio REQUIRED)
TARGET_LINK_LIBRARIES(${CMAKE_PROJECT_NAME} SFML::Audio)

# Get the GLEW environment variable.
SET(GLEW_DIR "$ENV{GLEW_DIR}")
IF(NOT GLEW_DIR)
  # The environment variable was not set
  SET(ERR_MSG "Please point the environment variable GLEW_DIR to the root directory of your GLEW installation.")
  IF(WIN32)
    # On Windows, try the default location
    MESSAGE(STATUS "Looking for GLEW in ${DEF_DIR_GLEW}")
    IF(IS_DIRECTORY ${DEF_DIR_GLEW})
      MESSAGE(STATUS "Found!")
      SET(GLEW_DIR ${DEF_DIR_GLEW})
    ELSE()
      MESSAGE(FATAL_ERROR ${ERR_MSG})
    ENDIF()
  ELSE()
    MESSAGE(FATAL_ERROR ${ERR_MSG})
  ENDIF()
ENDIF()
INCLUDE_DIRECTORIES(${GLEW_DIR}/include)
IF(WIN32)
  # With prebuilt binaries
  # Check for 32 vs 64 bit generator
  IF(NOT CMAKE_CL_64)
    MESSAGE(STATUS "Using 32Bit")
    TARGET_LINK_LIBRARIES(${CMAKE_PROJECT_NAME} ${GLEW_DIR}/lib/Release/Win32/glew32s.lib)
  ELSE()
    MESSAGE(STATUS "Using 64Bit")
    TARGET_LINK_LIBRARIES(${CMAKE_PROJECT_NAME} ${GLEW_DIR}/lib/Release/x64/glew32s.lib)
  ENDIF()
ELSE()
  TARGET_LINK_LIBRARIES(${CMAKE_PROJECT_NAME} ${GLEW_DIR}/lib/libGLEW.a)
ENDIF()

# Get the EIGEN environment variable. Since EIGEN is a header-only library, we
# just need to add it to the include directory.
SET(EIGEN3_INCLUDE_DIR "$ENV{EIGEN3_INCLUDE_DIR}")
IF(NOT EIGEN3_INCLUDE_DIR)
  # The environment variable was not set
  SET(ERR_MSG "Please point the environment variable EIGEN3_INCLUDE_DIR to the root directory of your EIGEN installation.")
  IF(WIN32)
    # On Windows, try the default location
    MESSAGE(STATUS "Looking for EIGEN in ${DEF_DIR_EIGEN}")
    IF(IS_DIRECTORY ${DEF_DIR_EIGEN})
      MESSAGE(STATUS "Found!")
      SET(EIGEN3_INCLUDE_DIR ${DEF_DIR_EIGEN})
    ELSE()
      MESSAGE(FATAL_ERROR ${ERR_MSG})
    ENDIF()
  ELSE()
    MESSAGE(FATAL_ERROR ${ERR_MSG})
  ENDIF()
ENDIF()
INCLUDE_DIRECTORIES(${EIGEN3_INCLUDE_DIR})

# Get FreeType 2 env var
FIND_PACKAGE(Freetype 2 REQUIRED)
# this defines:
#   FREETYPE2_FOUND        — TRUE if found
#   FREETYPE2_INCLUDE_DIRS — list of include dirs
#   FREETYPE2_LIBRARIES    — list of libraries to link

IF (NOT FREETYPE_FOUND)
  MESSAGE(FATAL_ERROR "Could not find FreeType2")
ENDIF()

# add FreeType headers to your target
TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME} PRIVATE ${FREETYPE_INCLUDE_DIRS})
# link against the FreeType library
TARGET_LINK_LIBRARIES(${PROJECT_NAME}  ${FREETYPE_LIBRARIES})

# Use c++17
SET_TARGET_PROPERTIES(${CMAKE_PROJECT_NAME} PROPERTIES CXX_STANDARD 17)
SET_TARGET_PROPERTIES(${CMAKE_PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)

# OS specific options and libraries
IF(WIN32)
  # -Wall produces way too many warnings.
  # -pedantic is not supported.
  # Disable warning 4996.
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4996")
  TARGET_LINK_LIBRARIES(${CMAKE_PROJECT_NAME} opengl32.lib)
  #TARGET_LINK_LIBRARIES(${CMAKE_PROJECT_NAME} glu32.lib) # Some people need this too
  SET_PROPERTY(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${CMAKE_PROJECT_NAME})
ELSE()
  # Enable all pedantic warnings.
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -pedantic")
  IF(APPLE)
    # Add required frameworks for GLFW.
    TARGET_LINK_LIBRARIES(${CMAKE_PROJECT_NAME} "-framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo")
  ELSE()
    #Link the Linux OpenGL library
    TARGET_LINK_LIBRARIES(${CMAKE_PROJECT_NAME} "GL")
  ENDIF()
ENDIF()

TARGET_PRECOMPILE_HEADERS(${CMAKE_PROJECT_NAME} PRIVATE "src/pch.h")
