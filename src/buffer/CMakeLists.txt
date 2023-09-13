add_library(
    db_buffer
    OBJECT
    buffer_pool_manager.cpp
    lru_k_replacer.cpp
    )

set(ALL_OBJECT_FILES
    ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:db_disk>
    PARENT_SCOPE)