#include "wtp.h"
#include "wtp_radio.h"

/* */
static int wtp_radio_configure_phy(struct wtp_radio* radio) {
	/* Default rate set is all supported rate */
	if (radio->radioid != radio->rateset.radioid) {
		if (radio->radioid != radio->supportedrates.radioid) {
			return -1;			/* Supported rate not set */
		}

		/* */
		radio->rateset.radioid = radio->radioid;
		radio->rateset.ratesetcount = radio->supportedrates.supportedratescount;
		memcpy(radio->rateset.rateset, radio->supportedrates.supportedrates, CAPWAP_RATESET_MAXLENGTH);
	}

	/* Check channel radio */
	if (radio->radioid != radio->radioinformation.radioid) {
		return -1;
	} else if (radio->radioid != radio->radioconfig.radioid) {
		return -1;
	} else if ((!radio->directsequencecontrol.radioid && !radio->ofdmcontrol.radioid) || ((radio->directsequencecontrol.radioid == radio->radioid) && (radio->ofdmcontrol.radioid == radio->radioid))) {
		return -1;		/* Only one from DSSS and OFDM can select */
	} else if ((radio->radioid == radio->directsequencecontrol.radioid) && !(radio->radioinformation.radiotype & (CAPWAP_RADIO_TYPE_80211B | CAPWAP_RADIO_TYPE_80211G))) {
		return -1;
	} else if ((radio->radioid == radio->ofdmcontrol.radioid) && !(radio->radioinformation.radiotype & CAPWAP_RADIO_TYPE_80211A)) {
		return -1;
	}

	return 0;
}

/* */
static void wtp_radio_destroy_wlan(struct wtp_radio_wlan* wlan) {
	if (wlan->wlanid && wlan->radio) {
		if (wlan->state != WTP_RADIO_WLAN_STATE_IDLE) {
			if (wlan->state == WTP_RADIO_WLAN_STATE_AP) {
				wifi_wlan_stopap(wlan->radio->radioid, wlan->wlanid);
			}

			/* Destroy interface */
			wifi_wlan_destroy(wlan->radio->radioid, wlan->wlanid);
		}
	}

	/* Release item */
	memset(wlan, 0, sizeof(struct wtp_radio_wlan));
}

/* */
void wtp_radio_init(void) {
	g_wtp.radios = capwap_array_create(sizeof(struct wtp_radio), 0, 1);
}

/* */
void wtp_radio_close(void) {
	int i, j;

	ASSERT(g_wtp.radios != NULL);

	for (i = 0; i < g_wtp.radios->count; i++) {
		struct wtp_radio* radio = (struct wtp_radio*)capwap_array_get_item_pointer(g_wtp.radios, i);

		if (radio->antenna.selections) {
			capwap_array_free(radio->antenna.selections);
		}

		if (radio->wlan) {
			for (j = 0; j < radio->wlan->count; j++) {
				wtp_radio_destroy_wlan((struct wtp_radio_wlan*)capwap_array_get_item_pointer(radio->wlan, j));
			}

			capwap_array_free(radio->wlan);
		}
	}

	capwap_array_resize(g_wtp.radios, 0);
}

/* */
void wtp_radio_free(void) {
	ASSERT(g_wtp.radios != NULL);
	ASSERT(g_wtp.radios->count == 0);

	if (g_wtp.events) {
		capwap_free(g_wtp.events);
	}

	capwap_array_free(g_wtp.radios);
}

/* */
int wtp_radio_setconfiguration(struct capwap_parsed_packet* packet) {
	int i;
	unsigned short binding;
	struct wtp_radio* radio;
	struct capwap_array* messageelements;

	ASSERT(packet != NULL);

	/* */
	binding = GET_WBID_HEADER(packet->rxmngpacket->header);
	if (binding == CAPWAP_WIRELESS_BINDING_IEEE80211) {
		struct capwap_list_item* search;

		/* Set radio configuration and invalidate the old values */
		search = packet->messages->first;
		while (search) {
			struct capwap_message_element_itemlist* messageelement = (struct capwap_message_element_itemlist*)search->item;

			/* Parsing only IEEE 802.11 message element */
			if (IS_80211_MESSAGE_ELEMENTS(messageelement->type)) {
				switch (messageelement->type) {
					case CAPWAP_ELEMENT_80211_DIRECTSEQUENCECONTROL: {
						messageelements = (struct capwap_array*)messageelement->data;
						if (messageelements && (messageelements->count > 0)) {
							struct capwap_80211_directsequencecontrol_element* directsequencecontrol;

							for (i = 0; i < messageelements->count; i++) {
								directsequencecontrol = *(struct capwap_80211_directsequencecontrol_element**)capwap_array_get_item_pointer(messageelements, i);
								radio = wtp_radio_get_phy(directsequencecontrol->radioid);
								if (radio) {
									memset(&radio->directsequencecontrol, 0, sizeof(struct capwap_80211_directsequencecontrol_element));
									memset(&radio->ofdmcontrol, 0, sizeof(struct capwap_80211_ofdmcontrol_element));
								}
							}
						}

						break;
					}

					case CAPWAP_ELEMENT_80211_OFDMCONTROL: {
						messageelements = (struct capwap_array*)messageelement->data;
						if (messageelements && (messageelements->count > 0)) {
							struct capwap_80211_ofdmcontrol_element* ofdmcontrol;

							for (i = 0; i < messageelements->count; i++) {
								ofdmcontrol = *(struct capwap_80211_ofdmcontrol_element**)capwap_array_get_item_pointer(messageelements, i);
								radio = wtp_radio_get_phy(ofdmcontrol->radioid);
								if (radio) {
									memset(&radio->directsequencecontrol, 0, sizeof(struct capwap_80211_directsequencecontrol_element));
									memset(&radio->ofdmcontrol, 0, sizeof(struct capwap_80211_ofdmcontrol_element));
								}
							}
						}

						break;
					}

					case CAPWAP_ELEMENT_80211_WTPRADIOINFORMATION: {
						messageelements = (struct capwap_array*)messageelement->data;
						if (messageelements && (messageelements->count > 0)) {
							struct capwap_80211_wtpradioinformation_element* radioinformation;

							for (i = 0; i < messageelements->count; i++) {
								radioinformation = *(struct capwap_80211_wtpradioinformation_element**)capwap_array_get_item_pointer(messageelements, i);
								radio = wtp_radio_get_phy(radioinformation->radioid);
								if (radio && (radio->radioid == radioinformation->radioid)) {
									memcpy(&radio->radioinformation, radioinformation, sizeof(struct capwap_80211_wtpradioinformation_element));
								}
							}
						}

						break;
					}
				}
			}

			/* Next */
			search = search->next;
		}

		/* Update new values */
		search = packet->messages->first;
		while (search) {
			struct capwap_message_element_itemlist* messageelement = (struct capwap_message_element_itemlist*)search->item;

			/* Parsing only IEEE 802.11 message element */
			if (IS_80211_MESSAGE_ELEMENTS(messageelement->type)) {
				switch (messageelement->type) {
					case CAPWAP_ELEMENT_80211_ANTENNA: {
						messageelements = (struct capwap_array*)messageelement->data;
						if (messageelements && (messageelements->count > 0)) {
							struct capwap_80211_antenna_element* antenna;

							for (i = 0; i < messageelements->count; i++) {
								antenna = *(struct capwap_80211_antenna_element**)capwap_array_get_item_pointer(messageelements, i);
								radio = wtp_radio_get_phy(antenna->radioid);
								if (radio && (radio->radioid == antenna->radioid)) {
									capwap_element_80211_antenna_copy(&radio->antenna, antenna);
								}
							}
						}

						break;
					}

					case CAPWAP_ELEMENT_80211_MACOPERATION: {
						messageelements = (struct capwap_array*)messageelement->data;
						if (messageelements && (messageelements->count > 0)) {
							struct capwap_80211_macoperation_element* macoperation;

							for (i = 0; i < messageelements->count; i++) {
								macoperation = *(struct capwap_80211_macoperation_element**)capwap_array_get_item_pointer(messageelements, i);
								radio = wtp_radio_get_phy(macoperation->radioid);
								if (radio && (radio->radioid == macoperation->radioid)) {
									memcpy(&radio->macoperation, macoperation, sizeof(struct capwap_80211_macoperation_element));
								}
							}
						}

						break;
					}

					case CAPWAP_ELEMENT_80211_MULTIDOMAINCAPABILITY: {
						messageelements = (struct capwap_array*)messageelement->data;
						if (messageelements && (messageelements->count > 0)) {
							struct capwap_80211_multidomaincapability_element* multidomaincapability;

							for (i = 0; i < messageelements->count; i++) {
								multidomaincapability = *(struct capwap_80211_multidomaincapability_element**)capwap_array_get_item_pointer(messageelements, i);
								radio = wtp_radio_get_phy(multidomaincapability->radioid);
								if (radio && (radio->radioid == multidomaincapability->radioid)) {
									memcpy(&radio->multidomaincapability, multidomaincapability, sizeof(struct capwap_80211_multidomaincapability_element));
								}
							}
						}

						break;
					}

					case CAPWAP_ELEMENT_80211_DIRECTSEQUENCECONTROL: {
						messageelements = (struct capwap_array*)messageelement->data;
						if (messageelements && (messageelements->count > 0)) {
							struct capwap_80211_directsequencecontrol_element* directsequencecontrol;

							for (i = 0; i < messageelements->count; i++) {
								directsequencecontrol = *(struct capwap_80211_directsequencecontrol_element**)capwap_array_get_item_pointer(messageelements, i);
								radio = wtp_radio_get_phy(directsequencecontrol->radioid);
								if (radio && (radio->radioid == directsequencecontrol->radioid)) {
									if (!(radio->radioinformation.radiotype & (CAPWAP_RADIO_TYPE_80211B | CAPWAP_RADIO_TYPE_80211G))) {
										return -1;
									}

									/* Configure radio channel */
									memcpy(&radio->directsequencecontrol, directsequencecontrol, sizeof(struct capwap_80211_directsequencecontrol_element));
									if (wifi_device_setfrequency(radio->radioid, WIFI_BAND_2GHZ, radio->radioinformation.radiotype, radio->directsequencecontrol.currentchannel)) {
										return -1;
									}
								}
							}
						}

						break;
					}

					case CAPWAP_ELEMENT_80211_OFDMCONTROL: {
						messageelements = (struct capwap_array*)messageelement->data;
						if (messageelements && (messageelements->count > 0)) {
							struct capwap_80211_ofdmcontrol_element* ofdmcontrol;

							for (i = 0; i < messageelements->count; i++) {
								ofdmcontrol = *(struct capwap_80211_ofdmcontrol_element**)capwap_array_get_item_pointer(messageelements, i);
								radio = wtp_radio_get_phy(ofdmcontrol->radioid);
								if (radio && (radio->radioid == ofdmcontrol->radioid)) {
									if (!(radio->radioinformation.radiotype & CAPWAP_RADIO_TYPE_80211A)) {
										return -1;
									}

									/* Configure radio channel */
									memcpy(&radio->ofdmcontrol, ofdmcontrol, sizeof(struct capwap_80211_ofdmcontrol_element));
									if (wifi_device_setfrequency(radio->radioid, WIFI_BAND_5GHZ, radio->radioinformation.radiotype, radio->ofdmcontrol.currentchannel)) {
										return -1;
									}
								}
							}
						}

						break;
					}

					case CAPWAP_ELEMENT_80211_RATESET: {
						messageelements = (struct capwap_array*)messageelement->data;
						if (messageelements && (messageelements->count > 0)) {
							struct capwap_80211_rateset_element* rateset;

							for (i = 0; i < messageelements->count; i++) {
								rateset = *(struct capwap_80211_rateset_element**)capwap_array_get_item_pointer(messageelements, i);
								radio = wtp_radio_get_phy(rateset->radioid);
								if (radio && (radio->radioid == rateset->radioid)) {
									memcpy(&radio->rateset, rateset, sizeof(struct capwap_80211_rateset_element));
								}
							}
						}

						break;
					}

					case CAPWAP_ELEMENT_80211_SUPPORTEDRATES: {
						messageelements = (struct capwap_array*)messageelement->data;
						if (messageelements && (messageelements->count > 0)) {
							struct capwap_80211_supportedrates_element* supportedrates;

							for (i = 0; i < messageelements->count; i++) {
								supportedrates = *(struct capwap_80211_supportedrates_element**)capwap_array_get_item_pointer(messageelements, i);
								radio = wtp_radio_get_phy(supportedrates->radioid);
								if (radio && (radio->radioid == supportedrates->radioid)) {
									memcpy(&radio->supportedrates, supportedrates, sizeof(struct capwap_80211_supportedrates_element));
								}
							}
						}

						break;
					}

					case CAPWAP_ELEMENT_80211_TXPOWER: {
						messageelements = (struct capwap_array*)messageelement->data;
						if (messageelements && (messageelements->count > 0)) {
							struct capwap_80211_txpower_element* txpower;

							for (i = 0; i < messageelements->count; i++) {
								txpower = *(struct capwap_80211_txpower_element**)capwap_array_get_item_pointer(messageelements, i);
								radio = wtp_radio_get_phy(txpower->radioid);
								if (radio && (radio->radioid == txpower->radioid)) {
									memcpy(&radio->txpower, txpower, sizeof(struct capwap_80211_txpower_element));
								}
							}
						}

						break;
					}

					case CAPWAP_ELEMENT_80211_WTP_QOS: {
						messageelements = (struct capwap_array*)messageelement->data;
						if (messageelements && (messageelements->count > 0)) {
							struct capwap_80211_wtpqos_element* qos;

							for (i = 0; i < messageelements->count; i++) {
								qos = *(struct capwap_80211_wtpqos_element**)capwap_array_get_item_pointer(messageelements, i);
								radio = wtp_radio_get_phy(qos->radioid);
								if (radio && (radio->radioid == qos->radioid)) {
									memcpy(&radio->qos, qos, sizeof(struct capwap_80211_wtpqos_element));
								}
							}
						}

						break;
					}

					case CAPWAP_ELEMENT_80211_WTP_RADIO_CONF: {
						messageelements = (struct capwap_array*)messageelement->data;
						if (messageelements && (messageelements->count > 0)) {
							struct capwap_80211_wtpradioconf_element* radioconfig;

							for (i = 0; i < messageelements->count; i++) {
								radioconfig = *(struct capwap_80211_wtpradioconf_element**)capwap_array_get_item_pointer(messageelements, i);
								radio = wtp_radio_get_phy(radioconfig->radioid);
								if (radio && (radio->radioid == radioconfig->radioid)) {
									memcpy(&radio->radioconfig, radioconfig, sizeof(struct capwap_80211_wtpradioconf_element));
								}
							}
						}

						break;
					}
				}
			}

			/* Next */
			search = search->next;
		}
	}

	return 0;
}

/* */
struct wtp_radio* wtp_radio_create_phy(void) {
	struct wtp_radio* radio;

	/* Create disabled radio */
	radio = (struct wtp_radio*)capwap_array_get_item_pointer(g_wtp.radios, g_wtp.radios->count);
	radio->radioid = (uint8_t)g_wtp.radios->count;
	radio->status = WTP_RADIO_DISABLED;

	/* Init configuration radio */
	radio->wlan = capwap_array_create(sizeof(struct wtp_radio_wlan), 0, 1);
	radio->antenna.selections = capwap_array_create(sizeof(uint8_t), 0, 1);
	return radio;
}

/* */
struct wtp_radio* wtp_radio_get_phy(uint8_t radioid) {
	int i;

	ASSERT(IS_VALID_RADIOID(radioid));

	for (i = 0; i < g_wtp.radios->count; i++) {
		struct wtp_radio* radio = (struct wtp_radio*)capwap_array_get_item_pointer(g_wtp.radios, i);
		if (radioid == radio->radioid) {
			return radio;
		}
	}

	return NULL;
}

/* */
struct wtp_radio_wlan* wtp_radio_get_wlan(struct wtp_radio* radio, uint8_t wlanid) {
	int i;

	ASSERT(IS_VALID_WLANID(wlanid));

	for (i = 0; i < radio->wlan->count; i++) {
		struct wtp_radio_wlan* wlan = (struct wtp_radio_wlan*)capwap_array_get_item_pointer(radio->wlan, i);
		if ((wlanid == wlan->wlanid) && (radio == wlan->radio)) {
			return wlan;
		}
	}

	return NULL;
}

/* */
static void wtp_radio_update_fdevent(void) {
	int count;

	/* Retrieve number of File Descriptor Event */
	count = wifi_event_getfd(NULL, NULL, 0);
	if (count < 0) {
		return;
	}

	/* */
	if (g_wtp.eventscount != count) {
		struct pollfd* fds;

		/* Resize poll */
		fds = (struct pollfd*)capwap_alloc(sizeof(struct pollfd) * (g_wtp.fdsnetworkcount + count));
		memcpy(fds, g_wtp.fds, sizeof(struct pollfd) * g_wtp.fdsnetworkcount);
		capwap_free(g_wtp.fds);
		g_wtp.fds = fds;

		/* Events Callback */
		if (g_wtp.events) {
			capwap_free(g_wtp.events);
		}

		g_wtp.events = (struct wifi_event*)((count > 0) ? capwap_alloc(sizeof(struct wifi_event) * count) : NULL);

		/* */
		g_wtp.eventscount = count;
		g_wtp.fdstotalcount = g_wtp.fdsnetworkcount + g_wtp.eventscount;
	}

	/* Retrieve File Descriptor Event */
	if (count > 0) {
		count = wifi_event_getfd(&g_wtp.fds[g_wtp.fdsnetworkcount], g_wtp.events, g_wtp.eventscount);
		ASSERT(g_wtp.eventscount == count);
	}
}

/* */
uint32_t wtp_radio_create_wlan(struct capwap_parsed_packet* packet, struct capwap_80211_assignbssid_element* bssid) {
	struct wtp_radio* radio;
	struct wtp_radio_wlan* wlan;
	struct capwap_80211_addwlan_element* addwlan;

	/* Get message elements */
	addwlan = (struct capwap_80211_addwlan_element*)capwap_get_message_element_data(packet, CAPWAP_ELEMENT_80211_ADD_WLAN);
	if (!addwlan) {
		return CAPWAP_RESULTCODE_FAILURE;
	}

	/* Get physical radio */
	radio = wtp_radio_get_phy(addwlan->radioid);
	if (!radio) {
		return CAPWAP_RESULTCODE_FAILURE;
	}

	/* Check if virtual interface is already exist */
	wlan = wtp_radio_get_wlan(radio, addwlan->wlanid);
	if (wlan) {
		return CAPWAP_RESULTCODE_FAILURE;
	}

	/* Prepare physical interface for create wlan */
	if (!radio->wlan->count) {
		if (wtp_radio_configure_phy(radio)) {
			return CAPWAP_RESULTCODE_FAILURE;
		}
	}

	/* Set virtual interface information */
	wlan = (struct wtp_radio_wlan*)capwap_array_get_item_pointer(radio->wlan, radio->wlan->count);
	wlan->radio = radio;
	wlan->wlanid = addwlan->wlanid;
	sprintf(wlan->wlanname, "%s%02d.%02d", g_wtp.wlanprefix, (int)addwlan->radioid, (int)addwlan->wlanid);
	if (wifi_iface_index(wlan->wlanname)) {
		memset(wlan, 0, sizeof(struct wtp_radio_wlan));
		return CAPWAP_RESULTCODE_FAILURE;
	}

	/* Create virtual interface */
	if (!wifi_wlan_create(addwlan->radioid, addwlan->wlanid, wlan->wlanname, NULL)) {
		wlan->state = WTP_RADIO_WLAN_STATE_CREATED;
	} else {
		wtp_radio_destroy_wlan(wlan);
		return CAPWAP_RESULTCODE_FAILURE;
	}

	/* Save wlan configuration */
	wlan->capability = addwlan->capability;
	wlan->qos = addwlan->qos;
	wlan->authmode = addwlan->authmode;
	wlan->macmode = addwlan->macmode;
	wlan->tunnelmode = addwlan->tunnelmode;
	wlan->ssid_hidden = addwlan->suppressssid;
	strcpy(wlan->ssid, (const char*)addwlan->ssid);
	wifi_wlan_getbssid(addwlan->radioid, addwlan->wlanid, wlan->bssid);

	/* TODO (struct capwap_array*)capwap_get_message_element_data(packet, CAPWAP_ELEMENT_80211_IE) */

	/* Configure virtual interface */
	if (!wifi_wlan_setupap(addwlan->radioid, addwlan->wlanid)) {
		wlan->state = WTP_RADIO_WLAN_STATE_READY;
	} else {
		wtp_radio_destroy_wlan(wlan);
		return CAPWAP_RESULTCODE_FAILURE;
	}

	/* Start AP */
	if (!wifi_wlan_startap(addwlan->radioid, addwlan->wlanid)) {
		wlan->state = WTP_RADIO_WLAN_STATE_AP;
	} else {
		wtp_radio_destroy_wlan(wlan);
		return CAPWAP_RESULTCODE_FAILURE;
	}

	/* Update Event File Descriptor */
	wtp_radio_update_fdevent();

	/* Retrieve macaddress of new device */
	bssid->radioid = addwlan->radioid;
	bssid->wlanid = addwlan->wlanid;
	memcpy(bssid->bssid, wlan->bssid, ETH_ALEN);

	return CAPWAP_RESULTCODE_SUCCESS;
}

/* */
uint32_t wtp_radio_update_wlan(struct capwap_parsed_packet* packet) {
	/* TODO */
	return CAPWAP_RESULTCODE_SUCCESS;
}

/* */
uint32_t wtp_radio_delete_wlan(struct capwap_parsed_packet* packet) {
	/* TODO */
	return CAPWAP_RESULTCODE_SUCCESS;
}
