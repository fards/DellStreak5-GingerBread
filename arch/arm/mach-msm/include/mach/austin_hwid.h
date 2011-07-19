#ifndef __AUSTIN_HWID_H_
#define __AUSTIN_HWID_H_

#if defined(CONFIG_HW_AUSTIN)
typedef enum {
	
	EVT0_Band125 = 0x00000000,
	EVT0_1_Band125,
	EVT0_1_Band18,
    	EVT1_Band125,
	EVT1_Band18,
	EVT1_4G_4G,
	EVT2_Band125,
	EVT2_Band18,
	EVT2P2_Band125,
	EVT2P2_Band18,
	EVT3_Band125,
	EVT3_Band18,
	EVT3P2_Band125,
	EVT3P2_Band18,
	EVT3P3_Band125,
	EVT3P3_Band18,
	DVT1_Band125,
	DVT1_Band18,
	EVT1_Band148,
	EVT2_Band148,
	EVT1_WiFiOnly,
	DVT2_Band125,
	DVT2_Band18,
	DVT1_Band148,
	DVT2_2_Band125,
	DVT2_2_Band18,
	DVT1_2_Band148,
	DVT2_Band148,
	DVT2_2_Band148,
	EVT1_2_WiFiOnly,
	EVT2_WiFiOnly,
	EVT2_2_WiFiOnly,
	EVT3_WiFiOnly,
	EVT3_2_WiFiOnly,
	DVT1_WiFiOnly,
	DVT1_2_WiFiOnly,
	DVT2_WiFiOnly,
	DVT2_2_WiFiOnly,
	TOUCAN_EVT1_Band125,
	TOUCAN_EVT1_Band148,
	TOUCAN_EVT2_Band125,
	TOUCAN_EVT2_Band148,
	TOUCAN_EVT3_Band125,
	TOUCAN_EVT3_Band148,
	HWID_UNKNOWN,
} HWID;
#elif defined(CONFIG_HW_TOUCAN)
typedef enum {
	
	TOUCAN_EVT1_Band125 = 0x00000000,
	TOUCAN_EVT1_Band148,
	TOUCAN_EVT2_Band125,
	TOUCAN_EVT2_Band148,
	TOUCAN_EVT3_Band125,
	TOUCAN_EVT3_Band148,
	TOUCAN_EVT3_2_Band125,
	TOUCAN_EVT3_2_Band148,
	TOUCAN_EVT3_4_Band125,
	TOUCAN_EVT3_4_Band148,
	TOUCAN_DVT1_Band125,
	TOUCAN_DVT1_Band148,
	HWID_UNKNOWN,
} HWID;
#endif

#endif