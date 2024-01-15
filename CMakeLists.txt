# CMakeList.txt : CMake project for lightBx, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.25)


# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("lightBx")

## get submodules from github
find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
## Update submodules as needed
    option(GIT_SUBMODULE "Check submodules during build" ON)
    if(GIT_SUBMODULE)
        message(STATUS "Submodule update")
        execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
                        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                        RESULT_VARIABLE GIT_SUBMOD_RESULT)
        if(NOT GIT_SUBMOD_RESULT EQUAL "0")
            message(FATAL_ERROR "git submodule update --init failed with ${GIT_SUBMOD_RESULT}, please checkout submodules")
        endif()
    endif()
endif()

##           CHECK ALL THE SUBMODULES
if(NOT EXISTS "${PROJECT_SOURCE_DIR}/external/glfw/CMakeLists.txt")
    message(FATAL_ERROR "The glfw submodules was not downloaded! GIT_SUBMODULE was turned off or failed. Please update submodules and try again.")
endif()


#Set vulkan variables
if (DEFINED VULKAN_SDK_PATH)
    message("FOUND VK")
    set(Vulkan_INCLUDE_DIRS "${VULKAN_SDK_PATH}/Include")
    set(Vulkan_LIBRARIES "${VULKAN_SDK_PATH}/Lib/vulkan-1.lib")
    set(Vulkan_FOUND "True")
else()
    find_package(Vulkan REQUIRED)
    message("${Vulkan_LIBRARIES}")
endif()


#file(
#    GLOB imgui_src 
#    "external/imgui/*.cpp" 
#    "external/imgui/backends/imgui_impl_glfw.cpp"
#    "external/imgui/backends/imgui_impl_opengl3.cpp"
#)

file(GLOB src
    "src/core/*.cpp"
    "src/math/*.cpp"
    "external/tiny_obj_loader/tiny_obj_loader.cpp"
    "external/stb_img/stb_image.cpp"
)


#Set asset paths
set(ASSET_DIR 
	" \"${CMAKE_CURRENT_LIST_DIR}/assets/\" "
)

set(SHADER_DIR 
    " \"${CMAKE_CURRENT_LIST_DIR}/assets/shaders/\" "
)

set(MODEL_DIR 
    " \"${CMAKE_CURRENT_LIST_DIR}/assets/models/\" "
)

set(IMAGE_DIR 
    " \"${CMAKE_CURRENT_LIST_DIR}/assets/textures/\" "
)

message("Using Asset directory: ${ASSET_DIR}")
message("Using Shader directory: ${SHADER_DIR}")
message("Using Model directory: ${MODEL_DIR}")
message("Using Image directory: ${IMAGE_DIR}")



configure_file(
    "${CMAKE_CURRENT_LIST_DIR}/src/core/settings.h.in"
    "${CMAKE_CURRENT_LIST_DIR}/src/core/settings.h"
)

# Add source to this project's executable.
add_executable (
    ${PROJECT_NAME}
    "src/main.cpp" 
    ${src}
)

add_subdirectory(external/glfw)
add_subdirectory(external/vk-bootstrap)
add_subdirectory(external/vma)


#TODO: Build shared libs for debugging?, but not release
OPTION( BUILD_SHARED_LIBS
  "Do not build package with shared libraries."
  OFF
)



target_include_directories(
	${PROJECT_NAME} 
	PUBLIC external/glfw/include
    PUBLIc external/vk-bootstrap/src
    PUBLIC external/tiny_obj_loader
    PUBLIC external/vma/include
    PUBLIC external/stb_img
    PUBLIC src
    PUBLIC ${Vulkan_INCLUDE_DIRS}
)

target_link_directories(
	${PROJECT_NAME} 
	PUBLIC external/glfw/src
    PUBLIC external/vma/
    PUBLIC ${Vulkan_LIBRARIES}
)

target_link_libraries(
	${PROJECT_NAME}
	glfw
    vk-bootstrap::vk-bootstrap
    ${Vulkan_LIBRARIES}
)

set_property(TARGET lightBx PROPERTY CXX_STANDARD 17)


# TODO: Add tests and install targets if needed.