.. role:: raw-html-m2r(raw)
   :format: html


Mmkv(Memory Key-Value database/cache)
=====================================


.. image:: https://img.shields.io/github/license/conzxy/mmkv
   :target: https://img.shields.io/github/license/conzxy/mmkv
   :alt: license

.. image:: https://img.shields.io/github/languages/top/conzxy/mmkv
   :target: https://img.shields.io/github/languages/top/conzxy/mmkv
   :alt: code-top

.. image:: https://img.shields.io/badge/Standard-C%2B%2B14-red
   :target: https://img.shields.io/badge/Standard-C%2B%2B14-red
   :alt: C++ standard
:raw-html-m2r:`<br>`
.. image:: https://img.shields.io/badge/Platform-Linux-blue
   :target: https://img.shields.io/badge/Platform-Linux-blue
   :alt: 
 
.. image:: https://img.shields.io/github/actions/workflow/status/conzxy/mmkv/cmake-linux.yml?label=Linux%20CI&logo=Linux
   :target: https://img.shields.io/github/actions/workflow/status/conzxy/mmkv/cmake-linux.yml?label=Linux%20CI&logo=Linux
   :alt: linux-build
:raw-html-m2r:`<br>`
.. image:: https://img.shields.io/github/v/tag/conzxy/mmkv
   :target: https://img.shields.io/github/v/tag/conzxy/mmkv
   :alt: version
 
.. image:: https://img.shields.io/github/last-commit/conzxy/mmkv
   :target: https://img.shields.io/github/last-commit/conzxy/mmkv
   :alt: commit-date
:raw-html-m2r:`<br>`

Introducation
-------------

``mmkv``\ 是一个基于\ **内存**\ 的\ **键值型**\ 数据库（或缓存），支持多种数据结构，包括


* 字符串(string)
* 列表(list)
* 有序集合(sorted set)
* 无序集合(hash set)
* 映射(map)

.. list-table::
   :header-rows: 1

   * - 数据类型
     - 实现
     - 对应数据结构
     - 源文件
   * - string
     - 支持动态增长的字符串
     - mmkv::algo::String
     - algo/string.h
   * - list
     - 无哨兵的双向链表（暂时）
     - mmkv::algo::Blist
     - algo/blist.h, algo/internal/bnode.h, algo/internal/blist_iterator.h
   * - sorted set(vset)
     - AvlTree与哈希表共同实现, AvlTree允许键(权重)重复而member是unique的，基于这个特性和一些命令的实现需要哈希表提供反向映射
     - mmkv::db::Vset
     - db/vset.h, algo/avl_tree.h, algo/internal/avl*.h, algo/internal/func_util.h, algo/dictionary.h
   * - hash set
     - 采用separate list实现的哈希表（支持Incremental rehash）
     - mmkv::algo::HashSet
     - algo/hash_set.h, slist.h, reserved_array.h, hash*.h, algo/internal/hash*.h
     - 
   * - map
     - 同hash set，不过元素类型是KeyValue
     - mmkv::algo::Dictionary
     - algo/dictionary.h, slist.h, reserved_array.h, hash*.h, algo/internal/hash*.h
   * - database instance
     - 以avl-tree作为list、基于separate-list实现的哈希表
     - mmkv::db::MmkvDb, mmkv::algo::AvlDictionary
     - algo/avl_dictionary.h, algo/internal/avl\ *.h, algo/internal/tree_hash*.h, db/kvdb.h


暂时没有考虑针对个别数据类型进行特化，比如满足一定的条件，list可以采用局部性更好的动态数组而不是链表等。

Schedule
--------

现在该项目还处于初级阶段，只是个单纯支持多个数据结构的单机单线程服务器，
尽管要支持多线程是较为简单的（因为kanon支持mutilthread-reactor），但是现在我暂时不关注这方面。

Server
^^^^^^


* [x] 支持\ ``string``\ 并实现其相关命令
* [x] 支持\ ``list``\ 并实现其相关命令
* [x] 支持\ ``sorted set``\ 并实现其相关命令
* [x] 支持\ ``map``\ 并实现其相关命令
* [x] 支持\ ``hash set``\ 并实现其相关命令
* [x] 支持\ ``Log``\ 和\ ``Recovery``\ （目前仅支持request log）
* [x] 实现key的\ ``expiration``\ 管理（包括是否允许设置过期时间，删除过期时间，过期键的检测策略）
* [x] 实现key的\ ``replacement``\ 管理（包括是否配置最大内存占用，达到最大内存占用时的替换策略）
* [x] 支持多线程处理请求
* [x] 支持分布式存储（暂时不完整）
* [ ] 支持Raft一致性算法，实现高可用

CLI
^^^


* [x] 实现\ ``translator``\ 将输入行翻译成MMBP request
* [x] 能够输入\ ``命令``\ (command)和\ ``参数``\ (arguments)与\ ``mmkv server``\ 进行交互
* [x] 能够对用户输入的命令的\ ``提示``\ (hint)，\ ``历史``\ (history)浏览，以及\ ``补全``\ (completion)
* [x] 实现\ `ternary-tree <https://github.com/Conzxy/ternary-tree>`_\ 以优化自动补全
* [x] 支持通过\ ``!command arguments``\ 执行\ ``shell``\ 命令
* [x] 对于输入的命令大小写不敏感（可以识别，提示，补全）

Client API
^^^^^^^^^^

参考\ `mmkvc <https://github.com/Conzxy/mmkvc>`_\ 。

Build
-----

CMake
^^^^^

.. code-block:: shell

   mkdir build
   cd build
   cmake ..
   cmake --build . --target mmkv-cli --parallel $(nproc)
   cmake --build . --target mmkv-server --parallel $(nproc)

Shell Script
^^^^^^^^^^^^

为了避免CMake的命令比较长，也可以通过以下脚本编译：

.. code-block:: shell

   cd bin
   chmod u+x build.sh
   ./build.sh mmkv-cli
   ./build.sh mmkv-server

FAQ
^^^


* 如果\ ``mmkv-cli``\ 输出了日志信息，可以通过设置环境变量关闭\ ``KANON_DEBUG=0``\ ，server的日志信息也可以通过相同的方法关闭。
