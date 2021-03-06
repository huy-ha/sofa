cmake_minimum_required(VERSION 3.12)
project(SofaSimulationCommon)

sofa_find_package(TinyXML REQUIRED)

set(HEADER_FILES
    config.h.in
    common.h
    init.h
    FindByTypeVisitor.h
    SceneLoaderPHP.h
    SceneLoaderXML.h
    TransformationVisitor.h
    xml/AttributeElement.h
    xml/BaseElement.h
    xml/BaseMultiMappingElement.h
    xml/DataElement.h
    xml/ElementNameHelper.h
    xml/NodeElement.h
    xml/ObjectElement.h
    xml/XML.h
)

set(SOURCE_FILES
    SceneLoaderPHP.cpp
    SceneLoaderXML.cpp
    TransformationVisitor.cpp
    init.cpp
    xml/AttributeElement.cpp
    xml/BaseElement.cpp
    xml/BaseMultiMappingElement.cpp
    xml/DataElement.cpp
    xml/ElementNameHelper.cpp
    xml/NodeElement.cpp
    xml/ObjectElement.cpp
    xml/XML.cpp
)

add_library(${PROJECT_NAME} SHARED ${HEADER_FILES} ${SOURCE_FILES})
target_link_libraries(${PROJECT_NAME} PUBLIC SofaCore SofaSimulationCore)
target_link_libraries(${PROJECT_NAME} PRIVATE tinyxml) # Private because not exported in API

sofa_add_targets_to_package(
    PACKAGE_NAME SofaSimulation
    TARGETS ${PROJECT_NAME} AUTO_SET_TARGET_PROPERTIES
    )
