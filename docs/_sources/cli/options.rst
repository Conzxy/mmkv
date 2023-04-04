.. _cli_options:

Cli Options
===========================================

CLI支持选项如下：

.. list-table::
   :header-rows: 1

   * - 选项
     - 作用
   * - -l/--log
     - 启动输出日志，一般用于调试
   * - -p/--port
     - 欲连接server的端口号
   * - -h/--host 
     - 欲连接server的域名/IP
   * - -r/--reconnect  
     - 当Server断开连接后，再次连接Server直到连接上为止
       
日志级别
----------
由于Mmkv的日志依赖于 `kanon <https://github.com/Conzxy/kanon>`_ 的日志模块。
所以日志级别分为：

* TRACE
* DEBUG
* INFO
* WARN
* ERROR
* FATAL
* OFF

用户可以通过 ``KANON_LOG`` 指定最低日志级别，小于该级别的日志将不会被输出的。

.. note::

   **FATAL** 是不能被禁止的，因为FATAL的行为是输出完成后终止程序，不允许被禁止。
