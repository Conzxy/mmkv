.. _cli_client:

CLI client
==============

``mmkv-cli``\ 是\ ``mmkv``\ 提供给用户与\ ``mmkv-server``\ 基于命令行进行交互的CLI程序。

Command-line edit
-----------------

由于本CLI想要实现\ ``自动补全``\ ，\ ``历史记录``\ ，\ ``命令提示``\ ，\ ``清屏``\ 这些命令行编辑需求，因此依赖于\ `replxx <https://github.com/AmokHuginnsson/replxx>`_\ 。

提示(Hint)
^^^^^^^^^^^^
.. image:: https://sm.ms/image/5KRXwbWfBaIOsyu
  :alt: hint

补全(Completion)
^^^^^^^^^^^^^^^^^^
.. image:: https://sm.ms/image/c8YThC1237Egntj
   :alt: completion

历史记录(History record)
^^^^^^^^^^^^^^^^^^^^^^^^^^

* HISTORY: 展示20个最新到最旧的历史记录
* CLEARHISTORY: 清空历史（包括内存和硬盘文件）

.. image:: https://sm.ms/image/cqRytFpgVKzIMB9
   :alt: history record

清屏(Clear screen)
^^^^^^^^^^^^^^^^^^^^^^

* CLEAR: 清空并刷新当前屏幕

.. image:: https://sm.ms/image/uL9T4lUx2FvzyOk
   :alt: clear scree

Shell-command
-------------

该CLI可以执行shell命令，只需要输入行以\ ``!``\ 开头，后面的输入内容会被视为shell命令及参数传递给\ ``/bin/shell``\ 等shell执行。

.. image:: https://sm.ms/image/fRUp2JslWgqVNmh
   :alt: shell command call

Mmkv-Command
----------------

通过与Mmkv对应的Command与Mmkv进行交互。
具体参考 :ref:`cli_commands` 。
