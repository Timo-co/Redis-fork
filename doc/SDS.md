# 简单动态字符串（simple dynamic string）

Redis 没有使用C语言传统的字符串表示（以空字符结尾的字符数组，以下简称C字符串），而是自己构建了一种名为简单的动态字符串的抽象类型，并将SDS用作Redis的默认字符串表示。


在Redis里面，C字符串只会作为字符串字面量用在一些无须对字符串值进行修改的地方，比如打印日志：

```
redisLog(REDIS_WARNING,"Redis is now ready to exit, bye bye...")
```

当Redis 需要的不仅仅时一个字符串字面量，而是一个可以被修改的字符串值时，Redis 就会使用SDS来表示字符串值，比如在Redis的数据库里面，包含字符串值的键值对在底层都是由SDS实现的。


举个例子，如果客户端执行命令：
```
redis> SET msg "hello world"
OK
```

那么Redis将在数据库中创建一个新的键值对，其中：

- 键值对的键是一个字符串对象，对象的底层实现是一个保存着字符串“msg” 的SDS

- 键值对的值也是一个字符串对象，对象的底层实现是一个保存着字符串“hello world”的SDS。

又比如，如果客户端执行命令：
```
Redis> RPUSH fruits "apple" "banana" "cherry"
(integer) 3
```

那么Redis 将在数据库中创建一个新的键值对，其中：

- 键值对的键是一个字符串的对象，对象的底层实现是一个保存了字符串"fruits" 的SDS。

- 键值对的值是一个列表对象，列表对象包含了三个字符串对象，这三个字符串对象分别由三个SDS实现：第一个SDS保存着"apple"，第二个SDS保存着字符串"banana", 第三个SDS保存着字符串"cherry"。

除了用来保存数据库的字符串值之外，SDS还被用作缓存区（buffer）：AOF 模块中的AOF缓存区，以及客户端状态中的输入缓存区，都是由SDS实现的，在之后介绍AOF持久化和客户端状态的时候，我们会看到SDS在这两个模块中的应用。

本章接下来将对SDS的实现进行介绍，说明SDS和C字符串的不同之处，解释为什么Redis 要使用SDS而不是C字符串，并在本章的最后列出SDS的操作API。



## 2.1 SDS的定义

每个sds.h/sdshdr 结构表述一个SDS的值：

```C
struct sdshdr{
    int len;
    int free;
    char buf[];
};
```

![](http://redisbook.com/_images/graphviz-72760f6945c3742eca0df91a91cc379168eda82d.png)


图2-1 展示了一个SDS示例：

- free 属性的值为0，表示这个SDS没有分配任何未使用空间。
- len 属性的值为5，表示这个SDS保存了一个五字节长的字符串。
- buf 属性是一个char类型的数组，数组的前五个字节分别保存了'R'、'e'、'd'、'i'、's'五个字符，而最后一个自己而保存了一个空字符'\0'。


SDS 遵循C字符串以空字符结尾的惯例，保存空字符的1字节空间不计算在SDS的len属性里面，并且为空字符分配额外的1字节空间，以及添加空字符到字符串末尾等操作，都是由SDS函数自动完成的，所以这个空字符对于SDS的使用者来说是完全透明的。遵循空字符结尾的好处是，SDS可以直接重用一部分C函数字符串函数库里面的函数。

举个例子，如果我们有一个指向图2-1所示SDS的指针s，那么可以直接使用<stdio.h>/printf 函数，通过执行 `print("%s",s->buf)` 来打印出SDS保存的字符串值"Redis"，而无须为SDS编写专门的打印函数。

![](http://redisbook.com/_images/graphviz-5fccf03155ec72c7fb2573bed9d53bf8f8fb7878.png)


图2-2 展示另一个SDS示例。这个SDS和之前展示的SDS一样，都保存了字符串值"Redis"。这个SDS和之前的SDS的区别在于，这个SDS为buf数组分配了五个字节未使用空间，所以它的free属性为5（图中使用五个空个表示五字节的未使用空间）。