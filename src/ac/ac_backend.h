#ifndef __AC_BACKEND_HEADER__
#define __AC_BACKEND_HEADER__

/* */
#define SOAP_NAMESPACE_URI					"http://smartcapwap/namespace"

/* SOAP event status*/
#define SOAP_EVENT_STATUS_GENERIC_ERROR		-1
#define SOAP_EVENT_STATUS_CANCEL			-2
#define SOAP_EVENT_STATUS_RUNNING			0
#define SOAP_EVENT_STATUS_COMPLETE			1

/* Reset notification */
struct ac_notify_reset_t {
	uint32_t vendor;
	uint8_t name[0];
};

/* Add WLAN notification */
struct ac_notify_addwlan_t {
	uint8_t radioid;
	uint8_t wlanid;
	uint16_t capability;
	uint8_t qos;
	uint8_t authmode;
	uint8_t macmode;
	uint8_t tunnelmode;
	uint8_t suppressssid;
	char ssid[CAPWAP_ADD_WLAN_SSID_LENGTH + 1];
};

/* Station Configuration IEEE802.11 add station notification */
struct ac_notify_station_configuration_ieee8011_add_station {
	uint8_t radioid;
	uint8_t address[MACADDRESS_EUI48_LENGTH];
	uint8_t vlan[CAPWAP_ADDSTATION_VLAN_MAX_LENGTH];

	uint8_t wlanid;
	uint16_t associationid;
	uint16_t capabilities;
	uint8_t supportedratescount;
	uint8_t supportedrates[CAPWAP_STATION_RATES_MAXLENGTH];
};

/* */
int ac_backend_start(void);
void ac_backend_stop(void);
void ac_backend_free(void);

/* */
int ac_backend_isconnect(void);
struct ac_http_soap_request* ac_backend_createrequest_with_session(char* method, char* uri);

#endif /* __AC_BACKEND_HEADER__ */
