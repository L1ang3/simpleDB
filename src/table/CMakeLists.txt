add_library(
    db_table
    OBJECT
    b_plus_tree_page.cpp
    )

set(ALL_OBJECT_FILES
    ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:db_table>
    PARENT_SCOPE)