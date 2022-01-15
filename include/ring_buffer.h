#pragma once

#include <NBR14522.h>

template <typename T, size_t S> class RingBuffer {
  private:
    std::array<T, S> _buffer;
    size_t _head = 0; // write index
    size_t _tail = 0; // read index
    size_t _toread = 0;

    void _increment_head() {
        _head++;
        if (_head == S)
            _head = 0;
    }

    void _increment_tail() {
        _tail++;
        if (_tail == S)
            _tail = 0;
    }

  public:
    inline void write(const T data) {
        _buffer.at(_head) = data;
        if (_head == _tail) {

            if (_toread == 0) {
                _increment_head();
                _toread++;
            } else {
                _increment_head();
                _increment_tail();
            }

        } else {
            _increment_head();
            _toread++;
        }
    }

    inline T read(void) {
        T retval = _buffer.at(_tail);

        if (_toread >= 1) {
            _toread--;
            _increment_tail();
        }

        return retval;
    }

    size_t toread() { return _toread; }
};
