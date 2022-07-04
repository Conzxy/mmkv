# Mmkv(In-Memory Key-Value database/cache)
## Introducation
mmkv是内存型键值数据库（或缓存），支持多种数据结构，包括
* 字符串（String）
* 列表（List）
* 无序集合（Hash set）
* 有序集合（Sorted set）
* 映射（Map）

不过现在只支持字符串。

存储键值对用的关键数据结构基本都是自造的轮子，这是由于STL是基于general-purpose设计的，而我们要的是specialized的数据结构，比如渐进式再哈希(`Incremental rehash`)的哈希表，无哨兵的单链表等(see `mmkv/algo`)，并且经量保证接口简洁易用。

## Build
项目依赖于[kanon](https://github.com/Conzxy/kanon)和[xxHash](https://github.com/Cyan4973/xxHash)。<br>
前者是我个人开发的网络库，后者是项目使用的哈希算法。<br>
安装参考各自的`github`页面中的`README.md`。<br>

```shell
git clone https://github.com/Conzxy/mmkv
cd mmkv/bin
export MMKV_BUILD_PATH=... # build目录的路径
# 或在~/.bash_profile中加上该句
chmod u+x release_build.sh
./release_build.sh mmkv_client
./release_build.sh mmkv_server
# Debug mode by following:
# chmod u+x build.sh
# ./build.sh mmkv_client
# ./build.sh mmkv_server
```

## Run
运行结果可以参考该GIF。<br>

### FAQ
* 如果`bin/mmkv_client`输出了日志信息，可以通过设置环境变量关闭`KANON_DEBUG=0`。
* 最终运行时若提示`xxHash`的动态库找不到，你可能需要输入`ldconfig`更新动态库缓存，以找到刚安装的`xxHash`(see [戳](https://stackoverflow.com/questions/480764/linux-error-while-loading-shared-libraries-cannot-open-shared-object-file-no-s))

![1.gif](https://s2.loli.net/2022/06/28/cB51DGmWg4APifl.gif)

## TODO
* Hash set支持
* Sorted set支持
* Map支持
* 持久化复原(Recovery)
* 分布式支持
* Raft容错支持
