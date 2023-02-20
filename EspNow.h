#include <Arduino.h>
#ifdef ESP32
#include <WiFi.h>
#include <esp_wifi.h>
#else
#include <ESP8266WiFi.h>
#endif
#include "MacAddress/MacAddress.h"
#include <Arduino_JSON.h>
#include <esp_now.h>
#define MAX_PEERS 20//20
int channel = 10;
int peerIndex = 0;
#define MAX_HANDLERS 15//15
Mac macHelper;
void onReceive(const uint8_t *mac, const uint8_t *data, int len);
String recievedData;
bool static espNowInit = false;
void defaultPrintHandlerESPNow(JSONVar msg)
{
	Serial.print("Default: ");
	Serial.println(msg["data"]);
	if(msg["newLine"])	Serial.println();
}
class Peer
{
public:
	JSONVar handleType;
	esp_now_peer_info_t *peer;
	int handlerIndex = 0;
	
	void (*dataRecieve[100])(JSONVar);
	Mac *peerAddress = new Mac();
	void init(String id)
	{
		this->peerAddress->parseName(id);
		if (!espNowInit)
		{
			InitESPNow();
		}
		addThisPeer();
		createPeer();
		// if(esp_now_register_recv_cb(onReceive) == ESP_OK)
		// {
		// 	Serial.println("callback");
		// }
		// Serial.println(peerAddress->getStrAddress());
		esp_now_register_recv_cb(onReceive);
		setOnRecieve(defaultPrintHandlerESPNow,"print");

	}
	void init(uint8_t id[])
	{
		this->peerAddress->setAddress(id);
		if (!espNowInit)
		{
			InitESPNow();
		}
		addThisPeer();
		createPeer();
		// if(esp_now_register_recv_cb(onReceive) == ESP_OK)
		// {
		// 	Serial.println("callback");
		// }
		// Serial.println(peerAddress->getStrAddress());
		esp_now_register_recv_cb(onReceive);
		setOnRecieve(defaultPrintHandlerESPNow,"print");

	}
	static void InitESPNow()
	{
		WiFi.mode(WIFI_STA);
		WiFi.disconnect();
		if (esp_now_init() == ESP_OK)
		{
			Serial.println("ESPNow Init Success");
			espNowInit = true;
		}
		else
		{
			Serial.println("ESPNow Init Failed");
			// Retry InitESPNow, add a counte and then restart?
			// InitESPNow();
			// or Simply Restart
			ESP.restart();
		}
	}
	void addThisPeer();

	void createPeer()
	{
		peer = new esp_now_peer_info_t();
		memcpy(peer->peer_addr, peerAddress->getAddress(), 6);
		// peer->channel=1;
		// peer->encrypt=false;
		// Register the peer
		if (esp_now_add_peer(peer) == ESP_OK)
		{
			Serial.println("Peer added");
		}
		// else if(esp_now_add_peer(peer) == ESP_ERR_ESPNOW_NOT_INIT)
		// {
		// 	Serial.println("not init");
		// }
		// else if(esp_now_add_peer(peer) == ESP_ERR_ESPNOW_ARG)
		// {
		// 	Serial.println("invalid arg");
		// }
		// else if(esp_now_add_peer(peer) == ESP_ERR_ESPNOW_NOT_FOUND)
		// {
		// 	Serial.println("not found");
		// }
	}
	void setOnRecieve(void (*f)(JSONVar), String type = "")
	{
		handleType[type] =handlerIndex;
		Serial.println("HandlerIndex: "+handlerIndex);
		Serial.println("Type: "+type);
		this->dataRecieve[handlerIndex++] = f;
	}
	void send(JSONVar data)
	{
		String dataString = JSON.stringify(data);
		sendString(dataString);
	}
	void sendString(String dataString)
	{
		const char *dataConst = dataString.c_str();
		int dataSize = dataString.length() + 1;
		char dataArray[dataSize];
		memcpy(dataArray, dataConst	, dataSize);
		// Serial.println(peerAddress->getStrAddress());
		esp_now_send(peerAddress->getAddress(), (uint8_t *)dataArray, dataSize);
		// if(esp_now_send(peerAddress->getAddress(), (uint8_t *)dataArray, dataSize) == ESP_OK)
		// {
		// 	Serial.println("sent success");
		// }
	}
	void println(String dataString)
	{
		print(dataString, true);
	}
	void print(String dataString, bool newLine = false)
	{
		JSONVar data;
		data["data"]=dataString;
		data["type"]="print";
		data["newLine"] = newLine;
		send(data);
	}
};

Peer *allPeers[MAX_PEERS];
void Peer::addThisPeer()
{
	allPeers[peerIndex++] = this;
}

Peer findPeer(String targetAddress)
{
	for (int i = 0; i < peerIndex; i++)
	{
		if (targetAddress == allPeers[i]->peerAddress->getStrAddress())
		{
			return *allPeers[i];
		}
	}
}
JSONVar recievedJson;
JSONVar stringData;
Peer dataFrom;
void onReceive(const uint8_t *src, const uint8_t *data, int len)
{
	recievedJson = JSONVar();
	macHelper.copyConstantUint(src);
	recievedData = "";
	
	
	for (int i = 0; i < len; i++)
	{
		recievedData += char(data[i]);
	}
	
	recievedJson = JSON.parse(recievedData);
	if(JSON.typeof(recievedJson) == "undefined"){
		recievedJson = JSONVar();
		recievedJson["type"] =  "String";
		recievedJson["value"] = recievedData;
		String type = "String";
	}
	String type = recievedJson["type"];
	dataFrom = findPeer(macHelper.getStrAddress());
	int typeIndex = dataFrom.handleType[type];
	typeIndex = typeIndex == -1 ? 0 : typeIndex;
	// Serial.print("typeIndex"+ String(typeIndex));
	dataFrom.dataRecieve[typeIndex](recievedJson);
}

void setId(String id)
{
	if (!espNowInit)
	{
		Peer::InitESPNow();
	}
	macHelper.parseName(id);
	// Serial.println(macHelper.getStrAddress());
	esp_wifi_set_mac(WIFI_IF_STA, macHelper.getAddress());
	// esp_base_mac_addr_set(macHelper.getAddress());
	// Serial.println("MAC: "+(String)WiFi.macAddress());
}

void setId(uint8_t id[])
{
	if (!espNowInit)
	{
		Peer::InitESPNow();
	}
	macHelper.setAddress(id);
	// Serial.println(macHelper.getStrAddress());
	esp_wifi_set_mac(WIFI_IF_STA, macHelper.getAddress());
	// esp_base_mac_addr_set(macHelper.getAddress());
	// Serial.println("MAC: "+(String)WiFi.macAddress());
}