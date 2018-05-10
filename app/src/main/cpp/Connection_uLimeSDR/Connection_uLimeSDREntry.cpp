/**
    @file Connection_uLimeSDREntry.cpp
    @author Lime Microsystems
    @brief Implementation of uLimeSDR board connection.
*/

#include <string>
#include <cstdlib>
#include <sstream>
#include "Connection_uLimeSDR.h"
#include "Logger.h"
#include <native-lib.h>
#include <log.h>
libusb_device_handle *tempDev_handle(nullptr);
using namespace lime;

namespace patch {
    template<typename T>
    std::string to_string(const T &n) {
        std::ostringstream stm;
        stm << n;
        return stm.str();
    }
}
int limeFd;
const char* usbDevPath;
int limeVid;
int limePid;

#ifdef __unix__
void Connection_uLimeSDREntry::handle_libusb_events()
{
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 250000;
    while(mProcessUSBEvents.load() == true)
    {
        int r = libusb_handle_events_timeout_completed(ctx, &tv, NULL);
        if(r != 0) lime::error("error libusb_handle_events %s", libusb_strerror(libusb_error(r)));
    }
}
#endif // __UNIX__

int Connection_uLimeSDR::USBTransferContext::idCounter=0;

//! make a static-initialized entry in the registry
void __loadConnection_uLimeSDREntry(void) //TODO fixme replace with LoadLibrary/dlopen
{
static Connection_uLimeSDREntry uLimeSDREntry;
}

Connection_uLimeSDREntry::Connection_uLimeSDREntry(void):
    ConnectionRegistryEntry("uLimeSDR")
{
#ifndef __unix__
    m_pDriver = new CDriverInterface();
#else
    int r = libusb_init(&ctx); //initialize the library for the session we just declared
    if(r < 0)
        LOGE("Init Error %i", r); //there was an error

    libusb_set_debug(ctx, 3); //set verbosity level to 3, as suggested in the documentation
/*//    LAM*/
//    libusb_set_option(ctx,LIBUSB_OPTION_LOG_LEVEL,3);
    mProcessUSBEvents.store(true);
    mUSBProcessingThread = std::thread(&Connection_uLimeSDREntry::handle_libusb_events, this);
#endif
}

Connection_uLimeSDREntry::~Connection_uLimeSDREntry(void)
{
#ifndef __unix__
    //delete m_pDriver;
#else
    mProcessUSBEvents.store(false);
    mUSBProcessingThread.join();
    libusb_exit(ctx);
#endif
}

std::vector<ConnectionHandle> Connection_uLimeSDREntry::enumerate(const ConnectionHandle &hint)
{
    std::vector<ConnectionHandle> handles;

#ifndef __unix__
    FT_STATUS ftStatus=FT_OK;
    static DWORD numDevs = 0;

    ftStatus = FT_CreateDeviceInfoList(&numDevs);

    if (!FT_FAILED(ftStatus) && numDevs > 0)
    {
        DWORD Flags = 0;
        char SerialNumber[16] = { 0 };
        char Description[32] = { 0 };
        for (DWORD i = 0; i < numDevs; i++)
        {
            ftStatus = FT_GetDeviceInfoDetail(i, &Flags, nullptr, nullptr, nullptr, SerialNumber, Description, nullptr);
            if (!FT_FAILED(ftStatus))
            {
                ConnectionHandle handle;
                handle.media = Flags & FT_FLAGS_SUPERSPEED ? "USB 3" : Flags & FT_FLAGS_HISPEED ? "USB 2" : "USB";
                handle.name = Description;
                handle.index = i;
                handle.serial = SerialNumber;
                handles.push_back(handle);
            }
        }
    }
#else
    libusb_device *devs = NULL; //pointer to pointer of device, used to retrieve a list of devices
    libusb_device **device;
    int r;
    int driver_active;
//    int usbDeviceCount = libusb_get_device_list(ctx, &devs);
    devs = libusb_get_device2(ctx,usbDevPath);

    libusb_device_descriptor desc;

    r = libusb_get_device_descriptor(devs, &desc);
    if(r<0)
        lime::error("failed to get device description");

    int pid = desc.idProduct;
    int vid = desc.idVendor;

//    if( vid == 0x1D50)
//    {
//        if(pid == 0x6108)
//        {
            if(libusb_open2(devs, &tempDev_handle,limeFd) != 0 || tempDev_handle == nullptr)
                return handles;
            ConnectionHandle handle;
            //check operating speed
            int speed = libusb_get_device_speed(devs);
            if(speed == LIBUSB_SPEED_HIGH)
                handle.media = "USB 2.0";
            else if(speed == LIBUSB_SPEED_SUPER)
                handle.media = "USB 3.0";
            else
                handle.media = "USB";
            //read device name
            char data[255];
            memset(data, 0, 255);
            int st = libusb_get_string_descriptor_ascii(tempDev_handle, LIBUSB_CLASS_COMM, (unsigned char*)data, 255);
            if(st < 0)
                lime::error("Error getting usb descriptor");
            else
                handle.name = std::string(data, size_t(st));
            handle.addr = patch::to_string(limePid)+":"+patch::to_string(limeVid);

            if (desc.iSerialNumber > 0)
            {
                r = libusb_get_string_descriptor_ascii(tempDev_handle,desc.iSerialNumber,(unsigned char*)data, sizeof(data));
                if(r<0)
                    lime::error("failed to get serial number");
                else
                    handle.serial = std::string(data, size_t(r));
            }
//            libusb_close(tempDev_handle);

            //add handle conditionally, filter by serial number
            if (hint.serial.empty() or hint.serial == handle.serial)
            {
                handles.push_back(handle);
            }
//        }
//    }

#endif
    return handles;
}

IConnection *Connection_uLimeSDREntry::make(const ConnectionHandle &handle)
{
#ifndef __unix__
    return new Connection_uLimeSDR(mFTHandle, handle.index);
#else
    const auto pidvid = handle.addr;
    const auto splitPos = pidvid.find(":");
//    LAM
    const auto pid = atoi(pidvid.substr(0, splitPos).c_str());
    const auto vid = atoi(pidvid.substr(splitPos+1).c_str());
    return new Connection_uLimeSDR(ctx, handle.index, vid, pid);
#endif
}
