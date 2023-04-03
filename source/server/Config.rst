.. role:: raw-html-m2r(raw)
   :format: html


配置文件默认是与\ ``mmkv-sever``\ 放在同一个目录的\ ``mmkv.conf``\ 。\ :raw-html-m2r:`<br>`
当然你也可以指定配置文件的位置：

.. code-block:: shell

   $ ./mmkv-server --config/-c location

Format
======

配置文件的格式是\ ``Field: Value``\ 行构成的

Field
=====


* ``LogMethod``

  * request: 写入\ ``MMBP``\ 请求，支持读取\ ``MMBP``\ 请求恢复DB
  * none(default): 不执行Log和Recover

* ``ExpirationCheckCycle``\ ：主动检测过期键的周期，如果不大于0，则不进行主动检测（默认：0秒）。
* ``RequestLogLocation``\ ：request log存放的位置（默认：/tmp/.mmkv-request.log）。
* ``LazyExpiration``\ : 懒惰检查过期键，即\ ``update/search``\ 某个键的时候，检查其是否过期。
* ``DiagnosticLogDirectory``\ : 诊断日志的存放目录
* ``ReplacePolicy``\ : 在执行\ ``update``\ 命令时，数据库达到最大内存占用的话，根据指定的替换算法释放键。

  * lru: Least-recently-used
  * none: 不论最大内存占用的指定，不进行替换

* ``MaxMemoryUsage``\ : 最大内存占用，格式为：\ ``usage[B/KB/MB/GB]``\ ，为0B时表示不限制内存占用（默认：0B）
