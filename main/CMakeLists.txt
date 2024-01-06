idf_component_register(
    SRCS "main.cpp"
    INCLUDE_DIRS ""
)

target_include_directories(${COMPONENT_TARGET}
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Add all source files from the "objects" directory
file(GLOB OBJECTS_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/objects/*.cpp")

# Add all source files from the "tasks" directory and its subdirectories
file(GLOB_RECURSE TASKS_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/tasks/*.cpp")

# Add additional source files
target_sources(${COMPONENT_TARGET} PRIVATE ${OBJECTS_SOURCES} ${TASKS_SOURCES})