add_library(
    db_disk
    OBJECT
    disk_manager.cpp
    page.cpp
    tuple.cpp
    page_guard.cpp
    )

set(ALL_OBJECT_FILES
    ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:db_disk>
    PARENT_SCOPE)