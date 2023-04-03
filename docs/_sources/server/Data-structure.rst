.. role:: raw-html-m2r(raw)
   :format: html


Data strucuture design & benchmark
==================================

``mmkv``\ 的数据结构都在\ ``/mmkv/algo``\ 中，主要包括：

* ReservedArray
* Slist
* Blist
* HashTable
* HashSet(HashTable derived class)
* Dictionary(HashTable wrapper)
* AvlTree
* TreeHashTable
* AvlDictionary(TreeHashTable derived class)

这些数据结构均是\ ``STL-like``\ 风格，区别在于API命名风格不同。

ReservedArray
-------------


.. image:: https://s2.loli.net/2022/08/09/MFo5b82DLdj1YzN.png
   :target: https://s2.loli.net/2022/08/09/MFo5b82DLdj1YzN.png
   :alt: reserved_array.png

:raw-html-m2r:`<br>`

   ``ReservedArray``\ 是专门为\ ``HashTable``\ 准备的连续容器，不过相对\ ``std::vector``\ ，不支持append操作自动扩展，因为只有\ ``data``\ 和\ ``end``\ 两个指针维护一个内存区域。\ :raw-html-m2r:`<br>`

除了这个区别，最大的区别就是\ ``ReservedArray``\ 支持\ ``reallocate``\ ，其默认采用的内存分配器是\ ``LibcAllocatorWithRealloc``\ （与STL的默认分配器的区别在于\ ``::realloc()``\ ）。具体来说，只有\ ``trivial``\ 类型和定义了静态数据成员\ ``static [constexpr] bool can_reallocate = true``\ 且默认构造函数不抛出异常的类才允许进行reallocate，其余的还是走老路子，即先分配新的内存区域再移动过去，释放原区域。\ :raw-html-m2r:`<br>`

.. image:: https://s2.loli.net/2022/08/09/8yv9gVtojfcmUrT.png
   :target: https://s2.loli.net/2022/08/09/8yv9gVtojfcmUrT.png
   :alt: reserved_array_slist.png

:raw-html-m2r:`<br>`

可见采用\ ``reallocate``\ 带来的性能提升是明显的

Slist
-----


.. image:: https://s2.loli.net/2022/08/09/PyjZU4oXDd8fkFA.png
   :target: https://s2.loli.net/2022/08/09/PyjZU4oXDd8fkFA.png
   :alt: slist.png

:raw-html-m2r:`<br>`

   ``Slist``\ 是与\ ``std::forward_list``\ 类似的单链表，区别在于\ ``Slist``\ 没有哨兵且没有成环（因为没有哨兵作为尾后迭代器），为了节省空间（哈希表分离列表法可能会有很多空槽占用哨兵）。

Blist
-----


.. image:: https://s2.loli.net/2022/08/09/iNEuyA4dw3oPB1k.png
   :target: https://s2.loli.net/2022/08/09/iNEuyA4dw3oPB1k.png
   :alt: blist.png

:raw-html-m2r:`<br>`

   ``Blist``\ 是类似\ ``std::list``\ 的双向链表，区别在于\ ``Blist``\ 没有哨兵且没有成环（同\ ``Slist``\ 的原因）。\ :raw-html-m2r:`<br>`

.. image:: https://s2.loli.net/2022/08/09/GnBskx36De8drbz.png
   :target: https://s2.loli.net/2022/08/09/GnBskx36De8drbz.png
   :alt: blist_int.png

:raw-html-m2r:`<br>`

（注意，图中橙色与棕色几乎合并）\ :raw-html-m2r:`<br>`

| Operation | Blist VS std::list | 
| --- | --- |
| popback | < |
| popfront | == | 
| pushback | > |
| pushfront | < |

虽然\ ``Blist``\ 从操作优劣程度来看，是比\ ``std::list``\ 差的，但是差距不大，作为省空间的代价是可以接受的。

HashTable
---------


.. image:: https://s2.loli.net/2022/08/09/mIlRe6nvDB3kNFL.png
   :target: https://s2.loli.net/2022/08/09/mIlRe6nvDB3kNFL.png
   :alt: hash_table.png

:raw-html-m2r:`<br>`

   ``HashTable``\ 是基于\ ``Separate-list``\ 策略的哈希表，列表类型是\ ``Slist``\ ，但是\ ``rehash``\ 策略并不是一次移动所有桶子(bucket)，而是每次读写操作移动一个桶子直到\ ``rehash``\ 完成，即所谓的\ ``递进式再散列``\ (Incremental rehash)。\ :raw-html-m2r:`<br>`

   除此之外，为了可以用\ ``&``\ 代替\ ``%``\ 获得桶的索引，\ ``rehash``\ 呈两倍扩展，而不是选择最近的更大素数，尽管这会导致即使选用的哈希函数比较均一，在一定程度上会导致碰撞率上升而降低性能。这点可以通过更换列表类型缓解\ :raw-html-m2r:`<br>`

不过该哈希表并没有针对迭代进行优化，因为一般这样做会降低\ ``点查询``\ (point query)的性能，因此暂时不考虑这方面的优化。

HashSet
-------

``HashSet``\ 是\ ``HashTable``\ 的子类，除了继承来的方法，还提供了三种方法支持对两个集合的\ ``交集``\ ，\ ``并集``\ ，\ ``差集``\ 的元素进行操作（传递回调）。

Dictionary
----------

``Dictionary``\ 是\ ``HashTable``\ 的子类，差别不大，仅新增了更方便键值对的方法。

AvlTree
-------

``mmkv``\ 的有序集合，我没有选择\ **红黑树**\ 或\ **跳表**\ ，首先，跳表的性能并没有平衡树好，其次，红黑树的高度并不是严格的\ ``O(lg(n))``\ 而是\ ``O(2lg(n+1))``\ 。因此对于\ ``读 > 写``\ 的\ ``mmkv``\ 而言高度更严格平衡的\ ``avl``\ 树更为适合。\ :raw-html-m2r:`<br>`

.. image:: https://s2.loli.net/2022/08/09/TiLdaDHNFlshc3q.png
   :target: https://s2.loli.net/2022/08/09/TiLdaDHNFlshc3q.png
   :alt: avl_tree.png

:raw-html-m2r:`<br>`

与红黑树相比，在插入和删除上更差，但是查询更好。符合预期结果。

TreeHashTable
-------------

``HashTable``\ 采用链表的弊端在\ ``HashTable``\ 中已经讲明。\ ``TreeHashTable``\ 选用的列表类型是\ ``平衡树``\ (balanced-search-tree)，可以使插入，删除，查询的算法复杂度维持在\ ``O(lgn)``\ ，在一定程度上缓解了由于哈希函数和表大小带来的问题。\ :raw-html-m2r:`<br>` 

AvlDictionary
-------------

``AvlDictionary``\ 是\ ``AvlTreeMap``\ (\ ``TreeHashTable``\ 的别名)的子类，列表类型是\ ``avl``\ 树。

.. image:: https://s2.loli.net/2022/08/09/ZqNiOS2WUIE18Gs.png
   :target: https://s2.loli.net/2022/08/09/ZqNiOS2WUIE18Gs.png
   :alt: avl_hash_table.png

:raw-html-m2r:`<br>`

与基于链表的哈希表相比，使用avl树的哈希表在查询和删除上要更好，但是插入要差一些。
