add_library(
    db_shell
    OBJECT
    shell.cpp
    )

set(ALL_OBJECT_FILES
    ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:db_shell>
    PARENT_SCOPE)

    # 添加你的项目源文件
add_executable(shell shell.cpp)

TARGET_LINK_LIBRARIES(shell sqlparser db)
