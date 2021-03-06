cmake_minimum_required(VERSION 3.12)
project(OptiTrackNatNet)


set(HEADER_FILES
    OptiTrackNatNetClient.h
    OptiTrackNatNetDevice.h
    config.h
)

set(SOURCE_FILES
    OptiTrackNatNetClient.cpp
    OptiTrackNatNetDevice.cpp
    initOptiTrackNatNet.cpp
)

find_package(SofaGeneral REQUIRED)
find_package(Boost QUIET COMPONENTS system regex date_time REQUIRED)

add_library(${PROJECT_NAME} SHARED ${HEADER_FILES} ${SOURCE_FILES})
set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "-DSOFA_BUILD_OPTITRACKNATNET")
target_link_libraries(${PROJECT_NAME} SofaCore SofaSimulationCommon SofaUserInteraction)
target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES})
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/..")

# Seems to be required by boost::asio
if(UNIX)
    target_link_libraries(${PROJECT_NAME} pthread)
endif()

install(TARGETS ${PROJECT_NAME}
        COMPONENT OptiTrackNatNet_libraries
        EXPORT OptiTrackNatNetTargets
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        PUBLIC_HEADER DESTINATION "include/${PROJECT_NAME}")

install(DIRECTORY examples/ DESTINATION share/sofa/plugins/${PROJECT_NAME})
