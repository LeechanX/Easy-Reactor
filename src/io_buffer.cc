#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <sys/uio.h>
#include <sys/ioctl.h>
#include "io_buffer.h"
#include "print_error.h"

#define EXTRA_MEM_LIMIT (5U * 1024 * 1024) //unit is K, so EXTRA_MEM_LIMIT = 5GB

io_buffer* buffer_pool::alloc(int N)
{
    int index;
    if (N <= u4K)
        index = u4K;
    else if (N <= u16K)
        index = u16K;
    else if (N <= u64K)
        index = u64K;
    else if (N <= u256K)
        index = u256K;
    else if (N <= u1M)
        index = u1M;
    else if (N <= u4M)
        index = u4M;
    else if (N <= u8M)
        index = u8M;
    else
        return NULL;

    ::pthread_mutex_lock(&_mutex);
    if (!_pool[index])
    {
        if (_total_mem + index / 1024 >= EXTRA_MEM_LIMIT)
        {
            exit_log("use too many memory");
            ::exit(1);
        }
        io_buffer* new_buf = new io_buffer(index);
        exit_if(new_buf == NULL, "new io_buffer");
        _total_mem += index / 1024;
        ::pthread_mutex_unlock(&_mutex);
        return new_buf;
    }
    io_buffer* target = _pool[index];
    _pool[index] = target->next;
    ::pthread_mutex_unlock(&_mutex);

    target->next = NULL;
    return target;
}

void buffer_pool::revert(io_buffer* buffer)
{
    int index = buffer->capacity;
    buffer->length = 0;
    buffer->head = 0;

    ::pthread_mutex_lock(&_mutex);

    assert(_pool.find(index) != _pool.end());
    buffer->next = _pool[index];
    _pool[index] = buffer;

    ::pthread_mutex_unlock(&_mutex);
}

buffer_pool::buffer_pool(): _total_mem(0)
{
    io_buffer* prev;

    _pool[u4K] = new io_buffer(u4K);
    exit_if(_pool[u4K] == NULL, "new io_buffer");

    prev = _pool[u4K];
    //4K buffer count: 5000 ≈ 20MB
    for (int i = 1;i < 5000; ++i)
    {
        prev->next = new io_buffer(u4K);
        exit_if(prev->next == NULL, "new io_buffer");
        prev = prev->next;
    }
    _total_mem += 4 * 5000;

    _pool[u16K] = new io_buffer(u16K);
    exit_if(_pool[u16K] == NULL, "new io_buffer");

    prev = _pool[u16K];
    //16K buffer count: 1000 ≈ 16MB
    for (int i = 1;i < 1000; ++i)
    {
        prev->next = new io_buffer(u16K);
        exit_if(prev->next == NULL, "new io_buffer");
        prev = prev->next;
    }
    _total_mem += 16 * 1000;

    _pool[u64K] = new io_buffer(u64K);
    exit_if(_pool[u64K] == NULL, "new io_buffer");

    prev = _pool[u64K];
    //64K buffer count: 500 ≈ 32MB
    for (int i = 1;i < 500; ++i)
    {
        prev->next = new io_buffer(u64K);
        exit_if(prev->next == NULL, "new io_buffer");
        prev = prev->next;
    }
    _total_mem += 64 * 500;

    _pool[u256K] = new io_buffer(u256K);
    exit_if(_pool[u256K] == NULL, "new io_buffer");

    prev = _pool[u256K];
    //256K buffer count: 100 ≈ 25MB
    for (int i = 1;i < 200; ++i)
    {
        prev->next = new io_buffer(u256K);
        exit_if(prev->next == NULL, "new io_buffer");
        prev = prev->next;
    }
    _total_mem += 256 * 200;

    _pool[u1M] = new io_buffer(u1M);
    exit_if(_pool[u1M] == NULL, "new io_buffer");

    prev = _pool[u1M];
    //1M buffer count: 50 = 50MB
    for (int i = 1;i < 50; ++i)
    {
        prev->next = new io_buffer(u1M);
        exit_if(prev->next == NULL, "new io_buffer");
        prev = prev->next;
    }
    _total_mem += 1024 * 50;

    _pool[u4M] = new io_buffer(u4M);
    exit_if(_pool[u4M] == NULL, "new io_buffer");

    prev = _pool[u4M];
    //4M buffer count: 20 = 80MB
    for (int i = 1;i < 20; ++i)
    {
        prev->next = new io_buffer(u4M);
        exit_if(prev->next == NULL, "new io_buffer");
        prev = prev->next;
    }
    _total_mem += 4096 * 20;

    _pool[u8M] = new io_buffer(u8M);
    exit_if(_pool[u8M] == NULL, "new io_buffer");

    prev = _pool[u8M];
    //4M buffer count: 10 = 80MB
    for (int i = 1;i < 10; ++i)
    {
        prev->next = new io_buffer(u8M);
        exit_if(prev->next == NULL, "new io_buffer");
        prev = prev->next;
    }
    _total_mem += 8192 * 10;
}

buffer_pool* buffer_pool::_ins = NULL;

pthread_mutex_t buffer_pool::_mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_once_t buffer_pool::_once = PTHREAD_ONCE_INIT;

void tcp_buffer::clear()
{
    if (_buf)
    {
        buffer_pool::ins()->revert(_buf);
        _buf = NULL;
    }
}

void tcp_buffer::pop(int len)
{
    assert(_buf && len <= _buf->length);
    _buf->pop(len);
    //buf is empty
    if (!_buf->length)
    {
        buffer_pool::ins()->revert(_buf);
        _buf = NULL;
    }
}

int input_buffer::read_data(int fd)
{
    //一次性读出来所有数据
    int rn, ret;
    if (::ioctl(fd, FIONREAD, &rn) == -1)
    {
        error_log("ioctl FIONREAD");
        return -1;
    }
    if (!_buf)
    {
        _buf = buffer_pool::ins()->alloc(rn);
        if (!_buf)
        {
            error_log("no idle for alloc io_buffer");
            return -1;            
        }
    }
    else
    {
        assert(_buf->head == 0);
        if (_buf->capacity - _buf->length < (int)rn)
        {
            //get new
            io_buffer* new_buf = buffer_pool::ins()->alloc(rn + _buf->length);
            if (!new_buf)
            {
                error_log("no idle for alloc io_buffer");
                return -1;
            }
            new_buf->copy(_buf);
            buffer_pool::ins()->revert(_buf);
            _buf = new_buf;
        }
    }

    do
    {
        ret = ::read(fd, _buf->data + _buf->length, rn);
    } while (ret == -1 && errno == EINTR);

    if (ret > 0)
    {
        assert(ret == rn);
        _buf->length += ret;
    }
    return ret;
}

int output_buffer::send_data(const char* data, int datlen)
{
    if (!_buf)
    {
        _buf = buffer_pool::ins()->alloc(datlen);
        if (!_buf)
        {
            error_log("no idle for alloc io_buffer");
            return -1;
        }
    }
    else
    {
        assert(_buf->head == 0);
        if (_buf->capacity - _buf->length < datlen)
        {
            //get new
            io_buffer* new_buf = buffer_pool::ins()->alloc(datlen + _buf->length);
            if (!new_buf)
            {
                error_log("no idle for alloc io_buffer");
                return -1;
            }
            new_buf->copy(_buf);
            buffer_pool::ins()->revert(_buf);
            _buf = new_buf;
        }
    }

    ::memcpy(_buf->data + _buf->length, data, datlen);
    _buf->length += datlen;
    return 0;
}

int output_buffer::write_fd(int fd)
{
    assert(_buf && _buf->head == 0);
    int writed;
    do
    {
        writed = ::write(fd, _buf->data, _buf->length);
    } while (writed == -1 && errno == EINTR);
    if (writed > 0)
    {
        _buf->pop(writed);
        _buf->adjust();
    }
    if (writed == -1 && errno == EAGAIN)
    {
        writed = 0;//不是错误，仅返回为0表示此时不可继续写
    }
    return writed;
}
