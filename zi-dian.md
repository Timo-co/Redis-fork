# 字典

### 4.1 字典的实现

Redis 的字典使用哈希表作为底层实现，一个哈希表中可以由多个哈希表节点，而每个哈希表节点就保存了字典中的一个键值对。

```cpp
/**
 * 字典
 * 每个字典使用两个哈希表，用于实现渐进式 rehash
*/
struct dict {
    // 特定类型的处理函数
    dictType *type;
    // 哈希表 2个
    dictEntry **ht_table[2];
    unsigned long ht_used[2];
    
    // 记录 rehash 进度，-1 表示rehash 未进行
    long rehashidx; /* rehashing not in progress if rehashidx == -1 */

    /* Keep small vars at end for optimal (minimal) struct padding */
    int16_t pauserehash; /* If >0 rehashing is paused (<0 indicates coding error) */
    signed char ht_size_exp[2]; /* exponent of size. (size = 1<<exp) */
};
```



#### 4.1.1 哈希表

Redis 字典所使用的哈希表由 dict.h/dictht 结构定义：
