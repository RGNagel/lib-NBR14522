#include <serial/serial_policy_unix.h>

#include <assert.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termio.h>
#include <unistd.h>

static speed_t _termiosBaudrate(baudrate_t baudrate) {
    switch (baudrate) {
    case BAUDRATE_110:
        return B110;
    case BAUDRATE_300:
        return B300;
    case BAUDRATE_600:
        return B600;
    case BAUDRATE_1200:
        return B1200;
    case BAUDRATE_2400:
        return B2400;
    case BAUDRATE_4800:
        return B4800;
    case BAUDRATE_9600:
        return B9600;
    case BAUDRATE_19200:
        return B19200;
    case BAUDRATE_38400:
        return B38400;
    case BAUDRATE_57600:
        return B57600;
    case BAUDRATE_115200:
        return B115200;
    default:
        assert(0);
    }
}

static int _termiosDatabits(databits_t databits) {
    switch (databits) {
    case DATABITS_5:
        return CS5;
    case DATABITS_6:
        return CS6;
    case DATABITS_7:
        return CS7;
    case DATABITS_8:
        return CS8;
    default:
        assert(0);
    }
}

static int _termiosParity(parity_t parity) {
    switch (parity) {
    case PARITY_NONE:
        return 0;
    case PARITY_ODD:
        return PARENB | PARODD;
    case PARITY_EVEN:
        return PARENB;
    default:
        assert(0);
    }
}

static int _termiosStopbits(stopbits_t stopbits) {
    switch (stopbits) {
    case STOPBITS_1:
        return 0;
    case STOPBITS_2:
        return CSTOPB;
    default:
        assert(0);
    }
}

static void _setTermiosOptions(struct termios& options, baudrate_t baudrate,
                               databits_t databits, parity_t parity,
                               stopbits_t stopbits) {
    // baudrate
    cfsetispeed(&options, _termiosBaudrate(baudrate));
    cfsetospeed(&options, _termiosBaudrate(baudrate));

    options.c_cflag |= CLOCAL | CREAD;

    // databits
    options.c_cflag |= _termiosDatabits(databits);
    // parity
    options.c_cflag |= _termiosParity(parity);
    // stopbits
    options.c_cflag |= _termiosStopbits(stopbits);

    // options.c_iflag |= (IGNPAR | IGNBRK);
}

bool SerialPolicyUnix::openSerial(const char* name, baudrate_t baudrate,
                                  databits_t databits, parity_t parity,
                                  stopbits_t stopbits) {
    _fd = open(name, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (_fd == -1)
        return false;

    struct termios options;
    tcgetattr(_fd, &options);
    bzero(&options, sizeof(options));
    _setTermiosOptions(options, baudrate, databits, parity, stopbits);
    // Activate the settings
    tcsetattr(_fd, TCSANOW, &options);
    return true;
}

void SerialPolicyUnix::closeSerial() {
    if (_fd != -1) {
        close(_fd);
        _fd = -1;
    }
}

SerialPolicyUnix::~SerialPolicyUnix() { closeSerial(); }

size_t SerialPolicyUnix::tx(const uint8_t* data, const size_t data_sz) {
#ifdef PRINT_BYTES
    printf("--> ");
    for (size_t i = 0; i < data_sz; i++)
        printf("%02X", data[i]);
    printf("\n");
#endif
    ssize_t numBytesWritten = write(_fd, data, data_sz);

    return numBytesWritten >= 0 ? numBytesWritten : 0;
}

size_t SerialPolicyUnix::rx(uint8_t* data, const size_t max_data_sz) {
    int toread = 0;
    ioctl(_fd, FIONREAD, &toread);
    if (toread <= 0)
        return 0;

    if (toread > static_cast<int>(max_data_sz))
        toread = max_data_sz;

    ssize_t numBytesRead = read(_fd, data, toread);

#ifdef PRINT_BYTES
    if (numBytesRead >= 0) {
        printf("<-- ");
        for (size_t i = 0; i < toread; i++)
            printf("%02X", data[i]);
        printf("\n");
    }
#endif

    return numBytesRead >= 0 ? numBytesRead : 0;
}