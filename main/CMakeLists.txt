idf_component_register(
    SRCS "main.cpp" "src/secrets.cpp"
    INCLUDE_DIRS ""
)

target_include_directories(${COMPONENT_TARGET}
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/lib
)
# Add all source files from the "objects" directory
file(GLOB OBJECTS_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/objects/*.cpp")

# Add all source files from the "tasks" directory and its subdirectories
file(GLOB_RECURSE TASKS_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/tasks/*.cpp")

# Add all source files from the "PubSubClient" directory
file(GLOB_RECURSE PUBSUB_SOURCES "A:/esp/esp-idf/components/arduino/libraries/PubSubClient/src/*.cpp")

# Add additional source files
target_sources(${COMPONENT_TARGET} PRIVATE ${OBJECTS_SOURCES} ${TASKS_SOURCES} ${PUBSUB_SOURCES})

include_directories("A:/esp/esp-idf/components/arduino/libraries/PubSubClient/src")