
Expiration Policy
=================


* ``懒惰检测``\ (Lazy check): 当client通过读命令获取内容和部分写命令修改内容时，进行对key的检测，如果过期了，直接回收内容并随后返回不存在。
* ``主动检测``\ (Active check): 以一定的周期(cycle)检测整个DB哪些key过期了。
  如果\ ``config``\ 指定\ ``LogMethod``\ 为\ ``request``\ ，那么检测到过期key的话，会向request log写入del key的请求内容以指示\ ``recovery``\ 删除

对于主动检测的周期，默认是\ ``60 seconds``\ ，可以通过\ ``config``\ 调整，\ ``-1``\ 表示不进行主动检测。

Implementation
==============

具体来说，显然存储\ ``<key -> expiration time>``\ 需要另开一个字典，因为不是所有key都有过期时间。

.. code-block:: cpp

   class MmkvDb {
     ...
     AvlDictionary<String, MmkvData> dict_;
     AvlDictionary<String, uint64_t> exp_dict_;
   };

还有一些相关方法，具体参考\ ``mmkd/db/kvdb.h``
