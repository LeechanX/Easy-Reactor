#ifndef __IO_BUFFER_H__
#define __IO_BUFFER_H__

#include <list>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <pthread.h>
#include <ext/hash_map>

struct io_buffer
{
    io_buffer(int size): 
    capacity(size), length(0), head(0), next(NULL)
    {
        data = new char[size];
        assert(data);
    }

    void clear()
    {
        length = head = 0;
    }

    void adjust()//move data to head
    {
        if (head)
        {
            if (length)
            {
                ::memmove(data, data + head, length);
            }
            head = 0;
        }
    }

    void copy(const io_buffer* other)
    {
        //only copy data to myself
        ::memcpy(data, other->data + other->head, other->length);
        head = 0;
        length = other->length;
    }

    void pop(int len)
    {
        length -= len;
        head += len;
    }

    int capacity;
    int length;
    int head;
    io_buffer* next;
    char* data;
};

class buffer_pool
{
public:
    enum MEM_CAP
    {
        u4K   = 4096,
        u16K  = 16384,
        u64K  = 65536,
        u256K = 262144,
        u1M   = 1048576,
        u4M   = 4194304,
        u8M   = 8388608
    };

    static void init()
    {
        _ins = new buffer_pool();
        assert(_ins);
    }

    static buffer_pool* ins()
    {
        pthread_once(&_once, init);
        return _ins;
    }

    io_buffer* alloc(int N);

    io_buffer* alloc() { return alloc(u4K); }

    void revert(io_buffer* buffer);

private:
    buffer_pool();

    buffer_pool(const buffer_pool&);
    const buffer_pool& operator=(const buffer_pool&);

    typedef __gnu_cxx::hash_map<int, io_buffer*> pool_t;
    pool_t _pool;
    uint64_t _total_mem;
    static buffer_pool* _ins;
    static pthread_mutex_t _mutex;
    static pthread_once_t _once;
};

struct tcp_buffer
{
    tcp_buffer(): _buf(NULL) { }

    ~tcp_buffer() { clear(); }

    const int length() const { return _buf? _buf->length: 0; }

    void pop(int len);

    void clear();

protected:
    io_buffer* _buf;
};

class input_buffer: public tcp_buffer
{
public:
    int read_data(int fd);

    const char* data() const { return _buf? _buf->data + _buf->head: NULL; }

    void adjust() { if (_buf) _buf->adjust(); }
};

class output_buffer: public tcp_buffer
{
public:
    int send_data(const char* data, int datlen);

    int write_fd(int fd);
};

#endif
