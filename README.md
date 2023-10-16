# simpleDB
一个简单的数据库系统

## 目录

- [功能特点](#功能特点)
- [安装](#安装)
- [使用示例](#使用示例)

## 功能特点

- **SQL 解析器**：使用了开源解析器[hyrise/sql-parser](https://github.com/hyrise/sql-parser)，将sql语句解析成c++对象。
- **内存管理**：基于LRU-K页面置换算法，实现了内存池管理系统。
- **存储引擎**：使用B+树作为数据存储结构。
- **执行引擎**：执行引擎采用火山模型设计，使用优化规则对执行计划进行优化。

## 安装
依赖项：g++、cmake、git、flex、bison

```git
git clone git@github.com:L1ang3/simpleDB.git
git submodule init
git submodule update

mkdir build
cd build
cmake ..
make

-- 运行测试确保正常运行
./bin/test
```

## 使用示例

```sql
-- 运行
./bin/shell

-- 创建表
CREATE TABLE users (id INT, name CHAR(255));

-- 插入数据
INSERT INTO users VALUES (1, 'John');
INSERT INTO users VALUES (2, 'Alice');

-- 查询数据
SELECT * FROM users;