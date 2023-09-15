# 添加测试执行器
enable_testing()

# 包含测试框架，例如Google Test
include(GoogleTest)

# 添加单元测试
add_executable(buffer_pool_manager_test buffer_pool_manager_test.cpp)

# 链接测试用例和被测试的模块
target_link_libraries(buffer_pool_manager_test gtest gtest_main db)

# # 添加测试，指定测试目标
# add_test(NAME my_unit_tests COMMAND unit_tests)

# # 添加集成测试
# add_executable(integration_tests test_integration.cpp)

# # 链接测试用例和被测试的模块
# target_link_libraries(integration_tests module1 module2)

# # 添加测试，指定测试目标
# add_test(NAME my_integration_tests COMMAND integration_tests)