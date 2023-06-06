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
    int free;
    int len;
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

接下来的一节将详细说明未使用空间的SDS中作用。

## 2.2 SDS 与 C 字符串的区别

根据传统，C语言使用长度为N+1的字符数组来表述长度为N的字符串，并且字符数组的最后一个元素总是空字符'\0'。

例如图2-3就展示了一个值为"Redis"的C字符串。


![](http://redisbook.com/_images/graphviz-cd9ca0391fd6ab95a2c5b48d5f5fbd0da2db1cab.png)

C语言使用这种简单的字符串表达方式，并不能满足Redis对字符串在安全性、效率以及功能方面的要求，本节接下来的内容将详细对比C字符串和SDS之间的区别，并说明SDS比C字符串更使用与Redis的原因。

### 2.2.1 常数复杂度获取字符串长度

因为C字符串并不记录自身的长度信息，所以为了获取C字符串的长度，程序必须遍历整个字符串，对遇到的每个字符进行计数，直到遇到代表字符串结尾的空字符为止，这个操作的复杂度为O(N)。

举个例子，图2-4 展示了程序计算一个C字符串长度的过程。

![](http://redisbook.com/_images/graphviz-11db45788777fdf62308ab859e5e4418276616b1.png)
![](http://redisbook.com/_images/graphviz-11db45788777fdf62308ab859e5e4418276616b1.png)
![](http://redisbook.com/_images/graphviz-22f91c392200e20da51ad61306765bee1d874a13.png)
![](http://redisbook.com/_images/graphviz-ff4aa6ea06fabe2bdf8f26325bca6a02fa480da8.png)
![](http://redisbook.com/_images/graphviz-59da530d8d0f16ce3eff95e285460b9ea5a0f389.png)
![](http://redisbook.com/_images/graphviz-e505478b940695671030146e66d6b3b292e7ae8c.png)

和C字符串不同，因为SDS在len属性中记录了SDS本身的长度，所以获取一个SDS的长度的复杂度仅为O(1)。

举个例子，对于图2-5所示的SDS来说，程序只要访问SDS的len属性，就可以立即知道SDS的长度为5字节。

![](http://redisbook.com/_images/graphviz-dbd2f4d49a9f495f18093129393569f93e645529.png)

又例如，对于图2-6展示的SDS来说，程序只要访问SDS的len属性，就可以立即知道SDS的长度为11字节。

![](http://redisbook.com/_images/graphviz-33b39668e26fa63350b177c13b38f201fcebb6c4.png)

设置和更新SDS长度的工作是由SDS的API在执行时自动完成的，使用SDS无需进行任何手动修改长度的工作。

通过使用SDS而不是C字符串，Redis将获取字符串的长度所需的复杂度从O(N)降低到了O(1)，这确保了获取字符串长度的工作不会成为Redis的性能瓶颈。例如，因为字符串键在底层使用SDS来实现，所以即使我们对一个非常长的字符串键反复执行STRLEN命令，也不会对系统性能造成任何影响，因为STRLEN 命令的复杂度仅为O(1)。


### 2.2.2 杜绝缓存区溢出
除了获取字符串长度复杂度高之外，C字符串比不记录自身长度带来的另一个问题时容易造成缓存区溢出（buffer overflow）。举个例子，<string.h>/strcat 函数可以将 src 字符串中的内容和拼接到dest 字符串末尾：

```C
char *strcat(char *dest,const char *src);
```
因为C字符串不记录自身长度，所以strcat假定用户在执行这个函数时，已经为dest分配了足够多的内存，可以容纳src字符串中的所有内容，而一旦这个假定不成立时，就会产生缓存区溢出。

举个例子，假设程序里有两个在内存中紧邻着的C字符串s1 和s2，其中s1 保存了字符串 "Redis"，而字符串s2 则保存了字符串"MongoDB"，如图2-7所示。

![](http://redisbook.com/_images/graphviz-7daf86931b270e1f4bacf20e3f56ebcb2fc7e08e.png)

假如一个程序员决定通过执行：`strcat(s1,"Cluster")` 将s1的内容修改为"Redis Cluster"，但是粗心的他忘可在执行strcat 之前给s1分配足够多的空间，那么在strcat函数执行之后，s1的数据将溢出到s2所在的空间中，导致s2保存的内容被意外修改，如图2-8 所示。

![](http://redisbook.com/_images/graphviz-2ff855d462d63f935deedb05c0d6447ed4b44bb3.png)

与C字符串不同，SDS的空火箭分配策略完全杜绝了发生缓存区溢出的可能性：当SDS API需要对SDS进行修改时，API会先检查SDS的空间是否满足修改所需的要求，如果不满足的话，API会自动将SDS的空间扩展至执行修改所需的大小，然后才执行实际的修改操作，所以使用SDS既不需要手动修改SDS的空间大小，也不会出现前面所说的缓存区溢出问题。