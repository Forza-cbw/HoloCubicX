#include "network.h"
#include "common.h"
#include <esp_wifi.h>
#include "HardwareSerial.h"

IPAddress local_ip(192, 168, 4, 2); // Set your server's fixed IP address here
IPAddress gateway(192, 168, 4, 2);  // Set your network Gateway usually your Router base address
IPAddress subnet(255, 255, 255, 0); // Set your network sub-network mask here
IPAddress dns(192, 168, 4, 1);      // Set your network DNS usually your Router base address

const char *AP_SSID = "HoloCubic_AIO"; // 热点名称
const char *HOST_NAME = "HoloCubic";   // 主机名

uint16_t ap_timeout = 0; // ap无连接的超时时间

TimerHandle_t xTimer_ap;

Network::Network()
{
    m_preDisWifiConnInfoMillis = 0;
    WiFi.enableSTA(false);
    WiFi.enableAP(false);
}

void Network::search_wifi(void)
{
    log_i("scan start");
    int wifi_num = WiFi.scanNetworks();
    log_i("scan done");
    if (0 == wifi_num)
    {
        log_i("no networks found");
    }
    else
    {
        log_i("%d",wifi_num);
        log_i(" networks found");
        for (int cnt = 0; cnt < wifi_num; ++cnt)
        {
            log_i("%d",cnt + 1);
            log_i(": ");
            log_i("%s",WiFi.SSID(cnt));
            log_i(" (");
            log_i("%d",WiFi.RSSI(cnt));
            log_i(")");
            log_i("%s",(WiFi.encryptionType(cnt) == WIFI_AUTH_OPEN) ? " " : "*");
        }
    }
}

boolean Network::start_conn_wifi(const char *ssid, const char *password)
{
    if (WiFi.status() == WL_CONNECTED)
    {
        log_i("\nWiFi is OK.\n");
        return false;
    }
    log_i("");
    log_i("Connecting: ");
    log_i("%s",ssid);
    log_i(" @ ");
    log_i("%s",password);

    // 设置为STA模式并连接WIFI
    WiFi.enableSTA(true);
    // 关闭省电模式 提升wifi功率（两个API都可以）
    // WiFi.setSleep(false);
    // esp_wifi_set_ps(WIFI_PS_NONE);
    // 修改主机名
    WiFi.setHostname(HOST_NAME);
    WiFi.begin(ssid, password);
    m_preDisWifiConnInfoMillis = GET_SYS_MILLIS();

    // if (!WiFi.config(local_ip, gateway, subnet, dns))
    // { //WiFi.config(ip, gateway, subnet, dns1, dns2);
    // 	log_i("WiFi STATION Failed to configure Correctly");
    // }
    // wifiMulti.addAP(AP_SSID, AP_PASS); // add Wi-Fi networks you want to connect to, it connects strongest to weakest
    // wifiMulti.addAP(AP_SSID1, AP_PASS1); // Adjust the values in the Network tab

    // log_i("Connecting ...");
    // while (wifiMulti.run() != WL_CONNECTED)
    // { // Wait for the Wi-Fi to connect: scan for Wi-Fi networks, and connect to the strongest of the networks above
    // 	delay(250);
    // 	log_i('.');
    // }
    // log_i("\nConnected to " + WiFi.SSID() + " Use IP address: " + WiFi.localIP().toString()); // Report which SSID and IP is in use
    // // The logical name http://fileserver.local will also access the device if you have 'Bonjour' running or your system supports multicast dns
    // if (!MDNS.begin(SERVER_NAME))
    // { // Set your preferred server name, if you use "myserver" the address would be http://myserver.local/
    // 	log_i(F("Error setting up MDNS responder!"));
    // 	ESP.restart();
    // }

    return true;
}

boolean Network::is_conn_wifi(void)
{
    if (WL_CONNECTED != WiFi.status())
    {
        if (doDelayMillisTime(10000, &m_preDisWifiConnInfoMillis, false))
        {
            // 这个if为了减少频繁的打印
            log_i("\nWiFi connect error.\n");
        }
        return false;
    }

    if (doDelayMillisTime(10000, &m_preDisWifiConnInfoMillis, false))
    {
        // 这个if为了减少频繁的打印
        log_i("\nWiFi connected");
        log_i("IP address: ");
        log_i("%s",WiFi.localIP());
    }
    return true;
}

// 关闭wifi和ap
boolean Network::close_wifi(void)
{
    if(WiFi.getMode() & WIFI_MODE_AP)
    {
        WiFi.enableAP(false);
        log_i("AP shutdowm");
    }

    if (!WiFi.disconnect())
    {
        return false;
    }
    WiFi.enableSTA(false);
    WiFi.mode(WIFI_MODE_NULL);
    // esp_wifi_set_inactive_time(ESP_IF_ETH, 10); //设置暂时休眠时间
    // esp_wifi_get_ant(wifi_ant_config_t * config);                   //获取暂时休眠时间
    // WiFi.setSleep(WIFI_PS_MIN_MODEM);
    // WiFi.onEvent();
    log_i("WiFi disconnect");
    return true;
}

boolean Network::open_ap(const char *ap_ssid, const char *ap_password)
{
    WiFi.enableAP(true); // 配置为AP模式
    // 修改主机名
    WiFi.setHostname(HOST_NAME);
    // WiFi.begin();
    boolean result = WiFi.softAP(ap_ssid, ap_password); // 开启WIFI热点
    if (result)
    {
        WiFi.softAPConfig(local_ip, gateway, subnet);
        IPAddress myIP = WiFi.softAPIP();

        // 打印相关信息
        log_i("\nSoft-AP IP address = ");
        log_i("%s",myIP);
        log_i("MAC address = %s" ,WiFi.softAPmacAddress().c_str());
        log_i("waiting ...");
        ap_timeout = 300; // 开始计时
        // xTimer_ap = xTimerCreate("ap time out", 1000 / portTICK_PERIOD_MS, pdTRUE, (void *)0, restCallback);
        // xTimerStart(xTimer_ap, 0); //开启定时器
    }
    else
    {
        // 开启热点失败
        log_i("WiFiAP Failed");
        return false;
        delay(1000);
        ESP.restart(); // 复位esp32
    }
    // 设置域名
    if (MDNS.begin(HOST_NAME))
    {
        log_i("MDNS responder started");
    }
    return true;
}

boolean Network::close_ap()
{
    // 关闭AP模式，但不影响STA（Wi-Fi客户端）
    WiFi.enableAP(false);
    log_i("AP shutdowm");
    // 关闭MDNS
    MDNS.end();
    log_i("MDNS responder shutdowm");
    return true;
}

void restCallback(TimerHandle_t xTimer)
{
    // 长时间不访问WIFI Config 将复位设备
    --ap_timeout;
    log_i("AP timeout: ");
    log_i("%d",ap_timeout);
    if (ap_timeout < 1)
    {
        // todo
        WiFi.softAPdisconnect(true);
        // ESP.restart();
    }
}