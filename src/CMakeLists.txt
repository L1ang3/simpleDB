add_subdirectory(disk)
add_subdirectory(shell)
add_subdirectory(table)
add_subdirectory(buffer)

add_library(db STATIC ${ALL_OBJECT_FILES})



target_link_libraries(
        db
        db_disk
        db_shell
        db_table
        db_buffer)
target_include_directories(
        db
        PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)