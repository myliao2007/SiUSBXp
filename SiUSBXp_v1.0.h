
#ifndef SIUSBXP
#define SIUSBXP
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <libusb-1.0/libusb.h>
#include <time.h>

/*Vendor ID / Product ID*/
#define		SI_USB_VID                      0x10c4
#define     SI_USB_PID                      0xea61

#ifdef DEBUG
#define DBG(args...) printf (args)
#else
#define DBG(format, ...)
#endif
#define ERR(args...) printf (args)

#define MAGIC 12939485
//#define BUF_SIZE 8192
#define BUF_SIZE SI_MAX_READ_SIZE

extern "C" void* _zb_usb_handle_status_on(void*);
extern "C" void* _zb_usb_handle_status_off(void*);

	extern "C" int LIBUSB_CALL device_in(
			libusb_context *ctx, 
			libusb_device *dev, 
			libusb_hotplug_event event, 
			void *user_data);
	extern "C" int LIBUSB_CALL device_out(
			libusb_context *ctx, 
			libusb_device *dev, 
			libusb_hotplug_event event, 
			void *user_data);

	struct user_data { 
		int plugin; /* 0: out 1: in -1: Not support the capability */
		int called;
	};

	class usb_event {

		private:
		int support_plugin;
		int vid;
		int pid;
		int class_id; 
		int chk_cap(void);
		libusb_context *ctx;

		public:
		struct user_data data;
		int handle_event(void);
		friend int LIBUSB_CALL device_in(libusb_context *ctx, 
			libusb_device *dev, 
			libusb_hotplug_event event, void *user_data);
		friend int LIBUSB_CALL device_ou(libusb_context *ctx, 
			libusb_device *dev, 
			libusb_hotplug_event event, void *user_data);
		usb_event(void);
		int init(void);
		int uninit(void);
		int register_event(void);
		int get_status(void);
		int set_status(int set);
		int set_vid_pid(int vid, int pid);
		~usb_event(void);
	};

class usb_handle {

	private:
	char hotplug_flag;
	libusb_device_handle *handle;
	libusb_device **dev_list;
	libusb_device *dev;
	libusb_context *ctx;
	libusb_device_descriptor info;
	libusb_config_descriptor *config;
	uint16_t vid;
	uint16_t pid;
	uint16_t class_id;
	int USB_Initialized;
	libusb_transfer *transfer_out;
	libusb_transfer *transfer_in;
	unsigned char ep_in;
	unsigned char ep_out;

	enum {
		CONTROL_REQUEST_TYPE_IN = (LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE),
		CONTROL_REQUEST_TYPE_OUT = (LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE),
		INTERRUPT_IN_ENDPOINT = (LIBUSB_ENDPOINT_IN|1),
		INTERRUPT_OUT_ENDPOINT = (LIBUSB_ENDPOINT_OUT|2),
		TIMEOUT = 5000,
		MAX_CONTROL_DATA = 64,
		MAX_INTERRUPT_SIZE = 64,
	};

	public:
	usb_event uv;
	int usb_set_debug(int);
	int SearchDev(int vid, int pid);
	int init(void);
	int GetNumDevices(void);
	int ShowDev(void);
	int GetDev(void);
	int open_usb(void);
	int close_usb(void);
	int SetVidPid(int _vid, int _pid);
	int PowerOn(void);
	int PowerOff(void);
	int read_usb(char *resp, size_t resp_len, size_t timeout);
	int write_usb(char *cmd, size_t cmd_len, size_t timeout);
	int get_speed(void);
	int set_rx_timeout(long rx);
	int set_tx_timeout(long tx);
	int rw_init(void);
	int CleanOTODevice(void);
	usb_handle(void);
	~usb_handle(void);
	#if 0
	friend void* _zb_usb_handle_status_on(void*);
	friend void* _zb_usb_handle_status_off(void*);
	int LIBUSB_CALL status_on(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data);
	int LIBUSB_CALL status_off(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data);
	#endif
};

