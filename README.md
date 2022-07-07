# Mmkv(Memory Key-Value database/cache)
## Introducation
`mmkv`是**内存**型**键值**数据库（或缓存），支持多种数据结构，包括
* 字符串（string）(√)
* 列表（list）(√)
* 有序集合（sorted set）(√)
* 无序集合（hash set）(×)
* 映射（map）(×)

存储键值对用的关键数据结构基本都是自造的轮子，这是由于STL是基于general-purpose设计的，而我们要的是specialized的数据结构，比如渐进式再哈希(`Incremental rehash`)的哈希表，无哨兵的单链表等(see `mmkv/algo`)，并且经量保证接口简洁易用。

| 数据类型 | 对应数据结构 | 源文件 | 说明 |
|---|---|---| ---|
| string | mmkv::algo::String | algo/string.h | 使用自定义allocator的std::string |
| list | mmkv::algo::Blist | algo/blist.h | 无哨兵的双向链表 |
| sorted set(vset) | mmkv::db::Vset | db/vset.h, algo/avl_tree.h, algo/internal/avl\*.h, algo/dictionary.h | 无header的Avl树，支持异质删除 |
| hash set | mmkv::algo::HashSet | algo/hash_set.h, slist.h, reserved_array.h, hash\*.h, internal/hash\*.h | 采用separate list实现的哈希表（支持Incremental rehash） |
| map | mmkv::algo::Dictionary | algo/dictionary.h | 同上，只不过元素类型为KeyValue |
全局哈希表可能之后会将链表更换为`Self-balanced binary search tree`(e.g. avltree, rbtree)

## Command
支持的命令：
* Common

|Command | Effect |
| --- | ---    |
| type key | 获取key的数据类型 |
| rename key new_name | 更换key，其数据结构不变 |
| del key | 删除key，无论其数据类型 |
| memorystat | 返回memoryfootprint信息，一般供CLI使用 |

* CLI(Command line interface)

| Command | Effect |
| --- | --- |
| quit/exit | 断开与mmkv server的连接，退出进程 |
| help | 查看help信息 |


* string

| Command | Effect |
| --- | --- |
| stradd key value | 添加string pair <key, value> |
| strget key | 获取key对应的value |
| strset key value | 修改key对应的value |
| strdel key | 删除key |

* list

| Command | Effect |
|--- | ---| 
| ladd key values... | 添加list <key, values...> |
| lappend key values... | 向key对应列表的尾部添加values |
| lprepend key values... | 向key对应列表的头部添加values |
| lpopback key count | 删除key对应列表的尾部count个元素（count比列表元素多时，删除全部）
| lpopfront key count | 删除key对应列表的头部count个元素 |
| lgetsize key | 获取key对应列表的元素个数 |
| lgetall key | 获取key对应列表的所有元素 |
| lgetrange key range(integer) | 获取key对应列表在range中的元素（range是左闭右开，即[left, right) )|

* sorted set

以下的range均是完全闭区间，即`[left, right]`

| Command | Effect |
| --- | --- |
| vadd key <weight,member>... | 添加<weight,member>...到key对应的有序集中，如果key不存在，则先创建 |
| vdelm key member | 删除key对应的有序集中对应的member |
| vdelmrange key order_range(integer) | 删除key对应的有序集中次序范围对应的元素 |
| vdelmrangebyweight key weight_range(double) | 删除key对应的有序集中权重范围对应的元素 | 
| vsize key | 获取key对应有序集的成员个数 |
| vsizebyweight key weight_range(double) | 获取key对应有序集在权重范围内的成员个数
| vweight key member | 获取member在key对应有序集中的权重 |
| vorder key member | 获取member在key对应有序集中的次序 |
| vrorder key member | 获取member在key对应有序集中的逆次序 |
| vrange key order_range(integer) | 获取在key对应有序集中次序范围内的成员和权重(<weight, member>s) |
| vrrange key order_range(integer) | 获取在key对应有序集中次序范围内的成员和权重（逆序） |
| vrangebyweight key weight_range(double) | 获取在key对应有序集中权重范围内的成员和权重 |
| vrrangebyweight key weight_range(double) | 获取在key对应有序集中权重范围内的成员和权重 （逆序） |

## Protocol
虽说CLI是输入的文本行，但实际是通过[translator](https://github.com/Conzxy/mmkv/blob/main/mmkv/client/translator.h)将文本行转换为自定义的二进制协议。
之所以采用二进制协议，有很多原因：
* 不需要对特殊字符进行特殊处理，比如空格，换行符
* 对于一些字段，二进制要更省空间

相对来说，文本协议的好处其实也是很明显的：
* 因为字符串居多，所以对应字段（比如key）不需要size header，在一些情况下会比二进制还省空间
* 文本协议是自解释的，因此扩展容易

尽管如此，我还是选用了二进制协议，是因为我想尝试一下自定义二进制协议（因此也没有用json，protobuf等流行序列化方式）。

具体来说，我将该协议命名为`MMBP(Memory binary protocol)`。
协议格式有两种（其实也可以合并为一种，但有些字段实际还是没必要，会占用多余的bit以表示是否设置）

#### MMBP request

![mmbp1.png](https://s2.loli.net/2022/07/07/eAowDHXYmOf4tuB.png)
* [`command`](https://github.com/Conzxy/mmkv/blob/main/mmkv/protocol/command.h): 命令类型（16bit的枚举类型）

除了`command`是**required**字段，其它均为**optional**字段
* `has_bits`: 表示optinoal字段哪些被设置
* `key_size`, key：key字符串

目前，有四种值类型
* `value`: 单value值，用于string命令和sorted set部分命令
* `value[]`: value数组，用于list命令和hash set命令（预定）
* `{ weight, member }[]`: { weight, member} 对数组，用于sorted set部分命令
* `{ key, value }[]`: 键值对数组，用于map命令（预定）

还有一些字段负责实现部分数据类型的部分命令：
* `count`: 32位整型，用于list pop命令
* `range`: 2个64位整型构成的范围，之所以是64位，是因为我想把`double`类型塞进去。只要通过`*(uint64_t*)&d`强转就能使`double`的bit mode不变，因此无关乎整型是否，只要底层表示不变。用于实现list和sorted set部分命令。

#### MMBP response

![mmbp2.png](https://s2.loli.net/2022/07/07/Kg9cIR3xJ2sm4Xz.png)
* `status_code`: 状态码，表示命令执行的结果。

| StatusCode | Meaning | 
| --- | --- |
| S_OK(0) | 执行成功 |
| S_NONEXISTS | 不存在该key |
| S_EXISTS | 已存在该key |
| S_INVALID_MESSAGE | 非法报文，不符合协议格式 |
| S_INVALID_RANGE | 非法范围 |
| S_VMEMBER_NONEXISTS | 该member不存在于有序集中 |

其他同request。

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
运行结果可以参考以下GIF。<br>

### FAQ
* 如果`mmkv_client`输出了日志信息，可以通过设置环境变量关闭`KANON_DEBUG=0`，server的日志信息也可以通过相同的方法关闭。
* 最终运行时若提示`xxHash`的动态库找不到，你可能需要输入`ldconfig`更新动态库缓存，以找到刚安装的`xxHash`(see [戳](https://stackoverflow.com/questions/480764/linux-error-while-loading-shared-libraries-cannot-open-shared-object-file-no-s))

#### string
![str_mmkv.gif](https://s2.loli.net/2022/07/07/5Zx69JDHMOzg3WF.gif)

#### list
![list_mmkv.gif](https://s2.loli.net/2022/07/07/XYLk8cp24OQzlDH.gif)

#### sorted set
![vset_mmkv.gif](https://s2.loli.net/2022/07/07/EpM1YRKg4GVNZky.gif)

## TODO
* 使用AvlTree实现新的哈希表
* 实现expire_time管理
* 实现replacement管理
* Hash set支持
* Map支持
* 持久化复原(Recovery)
* 分布式支持
* Raft容错支持