#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <sys/uio.h>
#include <sys/ioctl.h>
#include "io_buffer.h"
#include "print_error.h"

#define EXTRA_MEM_LIMIT (5U * 1024 * 1024) //unit is K, so EXTRA_MEM_LIMIT = 5GB

io_buffer* buffer_pool::alloc(uint32_t N)
{
    uint32_t index;
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

int input_buffer::read_data(int fd)
{
    uint32_t rn;
    if (::ioctl(fd, FIONREAD, &rn) == -1)
    {
        error_log("ioctl FIONREAD");
        return -1;
    }
    if (!_buf)
    {
        _buf = buffer_pool::ins()->alloc(rn);
        error_if(_buf == NULL, "no idle for alloc io_buffer");
        return -1;//need to close connection

        int rd = ::read(fd, _buf->data, rn);
        if (rd == -1)
        {
            error_log("read tcp socket");
            return -1;
        }
        else if (rd == 0)
        {
            //The peer is closed, return -2
            return -2;
        }
        else
        {
            assert(rn == (uint32_t)rd);
            //must read out rn bytes
            _buf->length = rn;
        }
    }
    else
    {
        if (_buf->capacity - _buf->length >= rn)
        {
            //can hold on
            //adjust original data to buf head
            _buf->adjust();

            int rd = ::read(fd, _buf->data + _buf->length, rn);
            if (rd == -1)
            {
                error_log("read tcp socket");
                return -1;
            }
            else if (rd == 0)
            {
                //The peer is closed, return -2
                return -2;
            }
            else
            {
                assert((uint32_t)rd == rn);
                //must read out rn bytes
                _buf->length += rn;
            }
        }
        else
        {
            //get new
            io_buffer* new_buf = buffer_pool::ins()->alloc(rn + _buf->length);
            error_if(new_buf == NULL, "no idle for alloc io_buffer");
            return -1;//need to close connection

            //_buf data move to new_buf
            new_buf->copy(_buf);
            buffer_pool::ins()->revert(_buf);
            _buf = new_buf;

            //append to _buf->length
            int rd = ::read(fd, _buf->data + _buf->length, rn);
            if (rd == -1)
            {
                error_log("read tcp socket");
                return -1;
            }
            else if (rd == 0)
            {
                //The peer is closed, return -2
                return -2;
            }
            else
            {
                assert((uint32_t)rd == rn);
                //must read out rn bytes
                _buf->length += rn;
            }
        }
    }
    return 0;
}

void input_buffer::pop(uint32_t len)
{
    assert(_buf && len <= _buf->length);
    _buf->length -= len;
    _buf->head += len;
    //buf is empty
    if (!_buf->length)
    {
        buffer_pool::ins()->revert(_buf);
        _buf = NULL;
    }
    else
    {
        ::memmove(_buf->data, _buf->data + _buf->head, _buf->length);
        _buf->head = 0;
    }
}

void input_buffer::clear()
{
    if (_buf)
    {
        buffer_pool::ins()->revert(_buf);
        _buf = NULL;
    }
}

void output_buffer::send_data(const char* data, uint32_t datlen)
{
    if (_buf_lst.empty())
    {
        //alloc for 4K
        int buf_cnt = datlen / buffer_pool::u4K;
        if (datlen % buffer_pool::u4K)
        {
            buf_cnt += 1;
        }
        for (int i = 0;i < buf_cnt; ++i)
        {
            io_buffer* new_buf = buffer_pool::ins()->alloc(buffer_pool::u4K);
            exit_if(new_buf == NULL, "alloc io_buffer");
            _buf_lst.push_back(new_buf);
        }
        buff_it it = _buf_lst.begin();
        int cp = 0;
        while (datlen)
        {
            io_buffer* buf = *it;
            if (datlen > buf->capacity)
            {
                ::memcpy(buf->data + cp, data + cp, buf->capacity);
                datlen -= buf->capacity;
                cp += buf->capacity;
                buf->length = buf->capacity;
            }
            else
            {
                ::memcpy(buf->data + cp, data + cp, datlen);
                buf->length = datlen;
                break;
            }
            ++it;
        }
    }
    else
    {
        io_buffer* buffer = _buf_lst.back();
        if (buffer->head)
        {
            ::memmove(buffer->data, buffer->data + buffer->head, buffer->length);
            buffer->head = 0;
        }
        if (buffer->capacity - buffer->length >= datlen)
        {
            ::memcpy(buffer->data + buffer->length, data, datlen);
            buffer->length += datlen;
        }
        else
        {
            //memcpy
            uint32_t left_len = buffer->capacity - buffer->length;
            ::memcpy(buffer->data + buffer->length, data, left_len);
            datlen -= left_len;
            data += left_len;

            //alloc for 4K
            int buf_cnt = datlen / buffer_pool::u4K;
            if (datlen % buffer_pool::u4K)
            {
                datlen += 1;
            }
            buff_it it;
            for (int i = 0;i < buf_cnt; ++i)
            {
                io_buffer* new_buf = buffer_pool::ins()->alloc(buffer_pool::u4K);
                exit_if(new_buf == NULL, "alloc io_buffer");
                _buf_lst.push_back(new_buf);
                if (i == 0)
                {
                    it = _buf_lst.end();
                    --it;
                }
            }

            int cp = 0;
            while (datlen)
            {
                io_buffer* buf = *it;
                if (datlen > buf->capacity)
                {
                    ::memcpy(buf->data + cp, data + cp, buf->capacity);
                    datlen -= buf->capacity;
                    cp += buf->capacity;
                    buf->length = buf->capacity;
                }
                else
                {
                    ::memcpy(buf->data + cp, data + cp, datlen);
                    buf->length = datlen;
                    break;
                }
                ++it;
            }
        }
    }
}

int output_buffer::write_fd(int fd)
{
    struct iovec iov[100];
    int iov_cnt = 0;
    if (_buf_lst.size() > 100)
    {
        error_log("output too large, can't write more now");
        return -2;
    }
    for (buff_it it = _buf_lst.begin();it != _buf_lst.end(); ++it)
    {
        io_buffer* buffer = *it;
        iov[iov_cnt].iov_base = buffer->data + buffer->head;
        iov[iov_cnt].iov_len = buffer->length;
        ++iov_cnt;
    }
    int wr = ::writev(fd, iov, iov_cnt);
    int wr_cnt = wr;
    while (wr_cnt > 0)
    {
        io_buffer* buffer = _buf_lst.front();
        if (buffer->length <= (uint32_t)wr_cnt)
        {
            wr_cnt -= buffer->length;
            buffer_pool::ins()->revert(buffer);
            _buf_lst.pop_front();
        }
        else
        {
            buffer->head += wr_cnt;
            break;
        }
    }
    return wr;
}

void output_buffer::clear()
{
    while (!_buf_lst.empty())
    {
        io_buffer* buffer = _buf_lst.front();
        buffer_pool::ins()->revert(buffer);
        _buf_lst.pop_back();
    }
}

uint32_t output_buffer::length() const
{
    uint32_t len = 0;
    for (cbuff_it it = _buf_lst.begin();it != _buf_lst.end(); ++it)
    {
        io_buffer* buffer = *it;
        len += buffer->length;
    }
    return len;
}
