#ifndef __CAPWAP_ELEMENT_80211_RATESET_HEADER__
#define __CAPWAP_ELEMENT_80211_RATESET_HEADER__

#define CAPWAP_ELEMENT_80211_RATESET		1034

#define CAPWAP_RATESET_MINLENGTH			2
#define CAPWAP_RATESET_MAXLENGTH			8

struct capwap_80211_rateset_element {
	uint8_t radioid;
	uint8_t ratesetcount;
	uint8_t rateset[CAPWAP_RATESET_MAXLENGTH];
};

extern struct capwap_message_elements_ops capwap_element_80211_rateset_ops;

#endif /* __CAPWAP_ELEMENT_80211_RATESET_HEADER__ */
