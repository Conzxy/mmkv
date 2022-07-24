# Mmkv(Memory Key-Value database/cache)
其他细节内容参考[wiki](https://github.com/Conzxy/mmkv/wiki)。

## Introducation
`mmkv`是一个基于**内存**的**键值型**数据库（或缓存），支持多种数据结构，包括
* 字符串(string)
* 列表(list)
* 有序集合(sorted set)
* 无序集合(hash set)
* 映射(map)

| 数据类型 | 实现 | 对应数据结构 | 源文件 |
|---|---|---|---|
| string | 支持动态增长的字符串 | mmkv::algo::String | algo/string.h |
| list | 无哨兵的双向链表（暂时）| mmkv::algo::Blist | algo/blist.h, algo/internal/bnode.h, algo/internal/blist_iterator.h |
| sorted set(vset) | AvlTree与哈希表共同实现, AvlTree允许键(权重)重复而member是unique的，基于这个特性和一些命令的实现需要哈希表提供反向映射| mmkv::db::Vset | db/vset.h, algo/avl_tree.h, algo/internal/avl\*.h, algo/internal/func_util.h, algo/dictionary.h |
| hash set | 采用separate list实现的哈希表（支持Incremental rehash） | mmkv::algo::HashSet | algo/hash_set.h, slist.h, reserved_array.h, hash\*.h, algo/internal/hash\*.h |  |
| map | 同hash set，不过元素类型是KeyValue | mmkv::algo::Dictionary | algo/dictionary.h, slist.h, reserved_array.h, hash\*.h, algo/internal/hash\*.h |
| database instance | 以avl-tree作为list、基于separate-list实现的哈希表 | mmkv::db::MmkvDb, mmkv::algo::AvlDictionary | algo/avl_dictionary.h, algo/internal/avl*.h, algo/internal/tree_hash*.h, db/kvdb.h |


暂时没有考虑针对个别数据类型进行特化，比如满足一定的条件，list可以采用局部性更好的动态数组而不是链表等。

## Schedule
现在该项目还处于初级阶段，只是个单纯支持多个数据结构的单机单线程服务器，
尽管要支持多线程是较为简单的（因为kanon支持mutilthread-reactor），但是现在我暂时不关注这方面。
* 支持string并实现其相关命令
* 支持list并实现其相关命令
* 支持sorted set并实现其相关命令
* 支持map并实现其相关命令
* 支持hash set并实现其相关命令
* 支持Recovery
* 支持Expire并实现其管理 <--
* 支持分布式存储(distributed system)
* 支持Raft容错
* 支持多线程

## Build
该项目需要额外安装[kanon](https://github.com/Conzxy/kanon)，是我个人编写的网络库，为该项目提供网络模块的支持。安装方式参考其github页面。

其他的依赖放在`third-party`，由项目本身管理。

```shell
git clone https://github.com/Conzxy/mmkv
cd mmkv/bin
export MMKV_BUILD_PATH=... # build目录的路径
# 或在~/.bash_profile中加上该句
chmod u+x release_build.sh
./release_build.sh mmkv-client
./release_build.sh mmkv-server
# Debug mode by following:
# chmod u+x build.sh
# ./build.sh mmkv-client
# ./build.sh mmkv-server
```

## Run
运行结果可以参考以下GIF。<br>

### FAQ
* 如果`mmkv-cli`输出了日志信息，可以通过设置环境变量关闭`KANON_DEBUG=0`，server的日志信息也可以通过相同的方法关闭。
* 最终运行时若提示`xxHash`的动态库找不到，你可能需要输入`ldconfig`更新动态库缓存，以找到刚安装的`xxHash`(see [戳](https://stackoverflow.com/questions/480764/linux-error-while-loading-shared-libraries-cannot-open-shared-object-file-no-s))
* 基于[linenoise](https://github.com/antirez/linenoise)，`mmkv-cli`可以提供**补全**(completion)，**提示**(hint)，**历史命令**(history)，**清屏**(clear screen)。

#### string
![str_mmkv.gif](https://s2.loli.net/2022/07/07/5Zx69JDHMOzg3WF.gif)

#### list
![list_mmkv.gif](https://s2.loli.net/2022/07/07/XYLk8cp24OQzlDH.gif)

#### sorted set
![vset_mmkv.gif](https://s2.loli.net/2022/07/07/EpM1YRKg4GVNZky.gif)

## TODO
* 分布式支持
* Raft容错支持
* 实现expire_time管理
* 实现replacement管理
