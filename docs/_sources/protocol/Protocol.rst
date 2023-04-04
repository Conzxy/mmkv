.. role:: raw-html-m2r(raw)
   :format: html

Mmbp protocol design
==============================

虽说CLI是输入的文本行，但实际是通过\ `translator <https://github.com/Conzxy/mmkv/blob/main/mmkv/client/translator.h>`_\ 将文本行转换为自定义的二进制协议。
之所以采用二进制协议，有很多原因：


* 不需要对特殊字符进行特殊处理，比如空格，换行符
* 对于一些字段，二进制要更省空间

相对来说，文本协议的好处其实也是很明显的：


* 因为字符串居多，所以对应字段（比如key）不需要size header，在一些情况下会比二进制还省空间
* 文本协议是自解释的，因此扩展容易

尽管如此，我还是选用了二进制协议，是因为我想尝试一下自定义二进制协议（因此也没有用json，protobuf等流行序列化方式）。

具体来说，我将该协议命名为\ ``MMBP(Memory binary protocol)``\ 。\ :raw-html-m2r:`<br>`
协议格式有两种（其实也可以合并为一种，但有些字段实际还是没必要，会占用多余的bit以表示是否设置）

MMBP request
------------


.. image:: https://s2.loli.net/2022/07/07/eAowDHXYmOf4tuB.png
   :target: https://s2.loli.net/2022/07/07/eAowDHXYmOf4tuB.png
   :alt: mmbp1.png



* `\ command <https://github.com/Conzxy/mmkv/blob/main/mmkv/protocol/command.h>`_\ : 命令类型（16bit的枚举类型）

除了\ ``command``\ 是\ **required**\ 字段，其它均为\ **optional**\ 字段


* ``has_bits``\ : 表示\ ``optinoal``\ 字段哪些被设置，一个byte可以表示8个\ ``optional``\ 字段
* ``key``\ ：key字符串

目前，有四种值类型


* ``value``\ : 单value值，用于string命令和sorted set/map/hash set部分命令
* ``value[]``\ : value数组，用于list命令和hash set/sorted set/map部分命令
* ``{ weight, member }[]``\ : { weight, member} 对数组，用于sorted set部分命令
* ``{ key, value }[]``\ : 键值对数组，用于map部分命令

还有一些字段负责实现部分数据类型的部分命令：


* ``count``\ : 32位整型，用于strpopback, list pop命令，sorted set的order相关命令等
* ``range``\ : 2个64位整型构成的范围，之所以是64位，是因为我想把\ ``double``\ 类型塞进去。只要通过\ ``*(uint64_t*)&d``\ 强转就能使\ ``double``\ 的bit mode不变，因此无关乎整型是否，只要底层表示不变。用于实现list和sorted set部分命令。

MMBP response
-------------


.. image:: https://s2.loli.net/2022/07/07/Kg9cIR3xJ2sm4Xz.png
   :target: https://s2.loli.net/2022/07/07/Kg9cIR3xJ2sm4Xz.png
   :alt: mmbp2.png



* ``status_code``\ : 状态码，表示命令执行的结果。\ :raw-html-m2r:`<br>`
  具体参考 `protocol/command.h <https://github.com/Conzxy/mmkv/blob/main/mmkv/protocol/command.h>`_ 。

其他同request。
