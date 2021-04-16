#include <usb_names.h>

#define MANUFACTURER_NAME {'t','u','f','f','r','a','b','i','t'}
#define MANUFACTURER_NAME_LEN 9

#define PRODUCT_NAME {'T','u','F','F','p','a','d'}
#define PRODUCT_NAME_LEN 7

#define SERIAL_NUMBER {'t','p','_','p','r','o','d'}
#define SERIAL_NUMBER_LEN 7

struct usb_string_descriptor_struct usb_string_manufacturer_name = {
  2 + MANUFACTURER_NAME_LEN * 2,
  3,
  MANUFACTURER_NAME};

struct usb_string_descriptor_struct usb_string_product_name = {
  2 + PRODUCT_NAME_LEN * 2,
  3,
  PRODUCT_NAME};

struct usb_string_descriptor_struct usb_string_serial_number = {
  2 + SERIAL_NUMBER_LEN * 2,
  3,
  SERIAL_NUMBER};
