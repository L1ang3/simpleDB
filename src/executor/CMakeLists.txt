add_library(
    db_executor
    OBJECT
    seq_scan_executor.cpp
    )

set(ALL_OBJECT_FILES
    ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:db_executor>
    PARENT_SCOPE)