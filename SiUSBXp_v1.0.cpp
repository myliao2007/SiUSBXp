
/* License: GNU */
/* author: Aaron Liao (myliao2007@gmail.com) */

//#define DEBUG

#ifndef SIUSBXP
#include <SiUSBXp_v1.0.h>
#endif

#include <iostream>

#define SI_SUCCESS	0

using namespace std;

static usb_handle usb_instance;

int usb_handle::get_speed(void)
{
	int ret;

	if( dev == NULL) {
		return -1;
	}

	ret = libusb_get_device_speed(dev);

	switch(ret) {

	case LIBUSB_SPEED_LOW:
		DBG("%s: 1.5", __func__);
		break;
	case LIBUSB_SPEED_FULL:
		DBG("%s: 12", __func__);
		break;
	case LIBUSB_SPEED_HIGH:
		DBG("%s: 480", __func__);
		break;
	case LIBUSB_SPEED_SUPER:
		DBG("%s: 5000", __func__);
		break;
	case LIBUSB_SPEED_UNKNOWN:
	default:
		DBG("%s:Unknown", __func__);
		break;
	}
	DBG(" MBit/s\n");
	
	return ret;
}

int usb_handle::init(void)
{
	if (!USB_Initialized) {

		USB_Initialized = 1;

		libusb_init(&ctx);

		if( libusb_has_capability(LIBUSB_CAP_HAS_HOTPLUG) ) {
			hotplug_flag = 1;
		}
	}

	return 0;
}

int usb_handle::CleanOTODevice(void)
{
	char buf[65];
	
	while ( read_usb(buf, 64, 1) > 0 ) {
		bzero(buf, sizeof buf);
	}

	return 0;
}

int usb_handle::read_usb(char *resp, size_t resp_len, size_t timeout)
{
	int read_num = 0;
	size_t total = 0;
	int ret = 0;
	int offset = 0;
	size_t MaxLen = 64;
	int count=0;
	int _status = 0;

	if( handle == NULL ) {
		return 0;
	}

	bzero(resp, resp_len+1);

	// chk usb device
	if( uv.get_status() != 1 ) {
		SearchDev(vid, pid);
	}

	do {

		_status =  uv.get_status();
		// usb device not plugin
		if( _status != 1 ) {
			#ifdef DEBUG
			printf("%s: HotPlug out\n", __func__);
			#endif
			return 0;
		}

		if( (resp_len - total) < MaxLen) {
			MaxLen = resp_len - total;
		}

		try {
			ret = libusb_bulk_transfer(
			handle, 
			ep_in, 
			(unsigned char*) resp + offset, MaxLen, &read_num, timeout);
		}
		catch (const char* msg) {
			#ifdef DEBUG
				printf("%s: msg: %s\n", __func__, msg);
			#endif
		}

		if( read_num <= 0 ) {

			_status =  uv.get_status();
			// usb device not plugin
			if( _status != 1 && SearchDev(vid, pid) != 0 ) {
				#ifdef DEBUG
				printf("%s: HotPlug out\n", __func__);
				#endif
				return 0;
			}

			count++;

			if( count > 3) {
				return 0;
			}

			_status =  uv.get_status();
			// usb device not plugin
			if( _status != 1 && SearchDev(vid, pid) != 0 ) {
				#ifdef DEBUG
				printf("%s: HotPlug out\n", __func__);
				#endif
				return 0;
			}

		}
		
		total += read_num;
		offset = total;

		DBG("%s: got total: %d\n", __func__, total);

	} while (total<resp_len);

	// clean extra byte
	for(int index=0; index<3; index++) {
		int _read_num;
		int _timeout = 1;
		char buf[64];
		ret = libusb_bulk_transfer(
			handle, 
			ep_in, 
			(unsigned char*) buf, 64, &_read_num, _timeout);
		DBG("ret[%d]: %d, len: %d\n", index, ret, _read_num);
	}

	for(size_t index=0; index<resp_len; index++) {
		//DBG("0x%02X ", (unsigned char)resp[index] );
	}
	DBG("\n");

	DBG("%s:ret: %d, read_len: %d\n", __func__, ret, total);

	return total;
}

int usb_handle::write_usb(char *cmd, size_t cmd_len, size_t timeout)
{
	int write_num = 0;
	int ret = 0;

	if( handle == NULL ) {
		return 0;
	}

	// usb device not plugin
	if( uv.get_status() != 1) {
		if( SearchDev(vid, pid) != 0 ) {
			return 0;
		}
	}

	CleanOTODevice();

	try {
		ret = libusb_bulk_transfer(
			handle, 
			ep_out, 
			(unsigned char*)cmd, cmd_len, &write_num, timeout);
	}

	catch (const char* msg) {
		#ifdef DEBUG
			printf("%s: msg: %s\n", __func__, msg);
		#endif
	}

	for(size_t index=0; index<cmd_len; index++){
		DBG("0x%X ", cmd[index] );
	}
	DBG("\n");

	DBG("%s:ret: %d, write_len: %d\n", __func__, ret, write_num);

	
	return write_num;
}

int usb_handle::PowerOn(void)
{
	libusb_set_configuration(handle, 1);
	return 0;
}

int usb_handle::PowerOff(void)
{
	libusb_set_configuration(handle, 0);
	return 0;
}

int usb_handle::SetVidPid(int _vid, int _pid)
{
	vid = _vid;
	pid = _pid;

	uv.set_vid_pid(vid, pid);
	uv.register_event();

	return 0;
}

int usb_handle::rw_init(void)
{
	libusb_control_transfer(handle, 0x40, 0x00, 0xFFFF, 0, NULL, 0, TIMEOUT);
	libusb_control_transfer(handle, 0x40, 0x02, 0x0002, 0, NULL, 0, TIMEOUT);

	/*
        // Send header to device 
        libusb_control_transfer(handle,
                                0x40, 0x00, 0xEEEE, 0xEEEE,
                                buf, 16, TIMEOUT);

		 // Notify device that all files are sent 
    		libusb_control_transfer(handle,
                            0x40, 0x00, 0xFFFF, 0xFFFF,
                            NULL, 0, TIMEOUT);
	*/


	return 0;
}

int usb_handle::open_usb(void)
{
	int ret;

	SearchDev(vid, pid);

	if( handle != NULL ) {
		close_usb();
		handle = NULL;
	}

	if( dev == NULL ) {
		DBG("%s usb device does not exist!\n", __func__);
		return -1;
	}

	libusb_open( dev, &handle);


	if( handle == NULL ) {
		DBG("%s Fail, handle is NULL\n", __func__);
		return -1;
	}

	//libusb_reset_device(handle);

		libusb_get_config_descriptor( dev, 0, &config);
		const libusb_interface *inter;
		const libusb_interface_descriptor *interdesc;
		const libusb_endpoint_descriptor *ep;

		for(int i=0; i<(int)config->bNumInterfaces; i++) {

			inter = &config->interface[i];

			for(int j=0; j<inter->num_altsetting; j++) {

				interdesc = &inter->altsetting[j];

					for(int k=0; k<(int)interdesc->bNumEndpoints; k++) {

						ep = &interdesc->endpoint[k];
						DBG("Descriptor Type: [k=%d] %d\n", k, (int) ep->bDescriptorType);
						DBG("EP address: %d\n", (int) ep->bEndpointAddress);

						if( k == 0) {
							ep_out = ep_out | (int) ep->bEndpointAddress;
							DBG("EP out address: %d\n", ep_out);
						} else if ( k == 1) {
							ep_in = ep_in | (int) ep->bEndpointAddress;
							DBG("EP in address: %d\n", ep_in);
						}
					}
			}
		}

	ret = libusb_kernel_driver_active( handle, 0);

	if( ret == 1 ) {
		DBG("%s::libusb_kernel_driver_active kernel driver detect\n", __func__);
		ret = libusb_detach_kernel_driver( handle, 0);
		DBG("%s::libusb_detach_kernel_driver::ret: %d\n", __func__, ret);
	}

	//libusb_set_configuration(handle, 1);

	ret = libusb_claim_interface(handle, 0);

	if( ret != 0 ) {
		DBG("%s fail ret=%d.\n", __func__, ret);
		return ret;
	}

	// LIBUSB_REQUEST_TYPE_VENDOR: 0x40
	// here causes stuck
	/*
	libusb_control_transfer(handle, 0x40, 0x00, 0xFFFF, 0, NULL, 0, TIMEOUT);
	libusb_clear_halt(handle, ep_in);
	libusb_clear_halt(handle, ep_out);
	libusb_control_transfer(handle, 0x40, 0x02, 0x0002, 0, NULL, 0, TIMEOUT);
	*/
	/*
        // Send header to device 
        libusb_control_transfer(handle,
                                0x40, 0x00, 0xEEEE, 0xEEEE,
                                buf, 16, TIMEOUT);

		 // Notify device that all files are sent 
    		libusb_control_transfer(handle,
                            0x40, 0x00, 0xFFFF, 0xFFFF,
                            NULL, 0, TIMEOUT);

		DBG("  USB Ctrl Message1 retval=%i\n", libusb_control_msg(dev, 0x40, 0x00, 0xFFFF, 0, NULL, 0, TIMEOUT));
		DBG("  USB Reset Endpoint IN retval=%i\n", libusb_resetep(dev, ep_in));
		DBG("  USB Reset Endpoint OUT retval=%i\n", libusb_resetep(dev, ep_out));
		DBG("  USB Clear Halt IN retval=%i\n", libusb_clear_halt(dev, ep_in));
		DBG("  USB Clear Halt OUT retval=%i\n", libusb_clear_halt(dev, ep_out));
		DBG("  USB Ctrl Message2 retval=%i\n", libusb_control_msg(dev, 0x40, 0x02, 0x0002, 0, NULL, 0, TIMEOUT));	
	*/

	return 0;
}

int usb_handle::close_usb(void)
{
	if( handle != NULL ) {
		libusb_release_interface(handle, 0);
		libusb_close(handle);
		handle = NULL;
	}
	
	return 0;
}

int usb_handle::usb_set_debug(int _level)
{
	libusb_set_debug(ctx, _level);
	return 0;
}

// return 0: if found the indicated device, else return -1
int usb_handle::SearchDev(int _vid, int _pid)
{
	int count;
	int ret = -1;
	libusb_device_descriptor _info;
	libusb_device **_devs = NULL;
	libusb_device_handle *_handle = NULL;

	if( dev_list != NULL) {
		libusb_free_device_list(dev_list, 1);
		dev_list = NULL;
		dev = NULL;
	}

	count = libusb_get_device_list(NULL, &_devs);

	if( count <= 0) {
		DBG("Error listing devices\n");
		return ret;
	}

	for( int index=0; index<count; index++) {

		libusb_get_device_descriptor(_devs[index], &_info);
		libusb_open(_devs[index], &_handle);

		if( _info.idVendor == _vid && _info.idProduct == _pid ) {
			ret = 0;
			dev = _devs[index];
			index=count; // to break for-loop
			DBG("%s: ret: %d\n", __func__, ret);
			uv.set_status(1);
		}

		libusb_close(_handle);
	}

	dev_list = _devs;

	return ret;
}

int usb_handle::ShowDev(void)
{
	unsigned char str[256];
	int count;
	libusb_device_descriptor _info;
	libusb_device **_devs = NULL;
	libusb_device_handle *_handle = NULL;

	count = libusb_get_device_list(NULL, &_devs);

	if( count <= 0) {
		DBG("Error listing devices\n");
		return -1;
	}

	for( int index=0; index<count; index++) {

		libusb_get_device_descriptor(_devs[index], &_info);
		libusb_open(_devs[index], &_handle);

		printf("===============================\n");
		printf("VID=0x%04X, PID=0x%04X\n", _info.idVendor, _info.idProduct);
		printf("Size of this descriptor (in bytes): %d\n", _info.bLength);
		printf("USB specification: ");


		switch(_info. bcdUSB) {

		case 0x0110:
			printf("USB 1.1\n");
			break;

		case 0x0200:
			printf("USB 2.0\n");
			break;

		case 0x0300:
			printf("USB 3.0\n");
			break;

		default:
			printf("USB 0x%X\n", _info.bcdUSB);
			break;

		}

		printf("Maximum packet size for endpoint 0: %d\n", _info.bMaxPacketSize0);
		printf("Device release number in binary-coded decimal: %d\n", _info.bcdDevice);
		printf("Index of string descriptor describing manufacturer: %d\n", _info.iManufacturer);

		bzero(str, sizeof str);
		libusb_get_string_descriptor_ascii(_handle, _info.iManufacturer, str, sizeof str);
		printf("manufacturer: %s\n", str);

		printf("Index of string descriptor describing product: %d\n", _info.iProduct);
		bzero(str, sizeof str);
		libusb_get_string_descriptor_ascii(_handle, _info.iProduct, str, sizeof str);
		printf("product: %s\n", str);

		printf("Index of string descriptor containing device serial number: %d\n", _info.iSerialNumber);
		bzero(str, sizeof str);
		libusb_get_string_descriptor_ascii(_handle, _info.iSerialNumber, str, sizeof str);
		printf("serial number: %s\n", str);

		printf("Number of possible configurations: %d\n", _info.bNumConfigurations);
	
		libusb_close(_handle);
	}

	if( _devs != NULL ) {
		libusb_free_device_list(_devs, 1);
	}

	return 0;
}

int usb_handle::GetNumDevices(void)
{
	int dev_num;
	libusb_device **_dev_list;

	dev_num = libusb_get_device_list(NULL, &_dev_list);

	if( _dev_list != NULL ) {
		libusb_free_device_list(_dev_list, 1);
	}

	return dev_num;
}

usb_handle::usb_handle(void)
{
	hotplug_flag = 0;
	libusb_get_version();
	handle = NULL;
	ctx = NULL;	
	dev_list = NULL;
	config = NULL;
	vid = 0x00;
	pid = 0x00;
	class_id = LIBUSB_HOTPLUG_MATCH_ANY;
	USB_Initialized = 0;
	transfer_out = NULL;
	transfer_in = NULL;
	memset(&info, 0, sizeof info);
	ep_in = LIBUSB_ENDPOINT_IN |
			LIBUSB_RECIPIENT_DEVICE | 
			LIBUSB_RECIPIENT_ENDPOINT ;
	ep_out = LIBUSB_ENDPOINT_OUT |
			LIBUSB_RECIPIENT_DEVICE | 
			LIBUSB_RECIPIENT_ENDPOINT ;
	init();

	transfer_in = libusb_alloc_transfer(0);
}

usb_handle::~usb_handle(void)
{
	close_usb();

	if( dev_list != NULL) {
		libusb_free_device_list(dev_list, 1);
		dev_list = NULL;
	}

	libusb_exit(ctx);
}

