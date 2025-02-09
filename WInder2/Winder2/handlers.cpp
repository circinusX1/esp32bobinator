#include "_my_config.h"
#include "eeprom.h"
#include "application.h"
#include "espxxsrv.h"
#include "sens_sht21.h"
#include "sens_sht3x.h"
#include "sens_aht20.h"
#include "css.h"

//////////////////////////////////////////////////////////////////////////////////////////
static String  WebPage;
static bool    FirstReq=false;

//////////////////////////////////////////////////////////////////////////////////////////
bool espxxsrv_t::_capturePage(RQ_TYPE_AND_PARAM)
{
    eeprom_t e;
    LOG(__FUNCTION__);

#if ASYNC_WEB
    WebPage = REQ_OBJECT.url();
#else
    WebPage = REQ_OBJECT.uri();
#endif
    String   apn = CFG(mcteck); apn += ".local";
#if ASYNC_WEB
    String   header = request->getHeader("host")->toString();
    if(header.isEmpty())
        header = request->getHeader("HOST")->toString();
#else
    String   header = REQ_OBJECT.hostHeader();
#endif
    if (!_isIp(header) && header != apn)
    {
#if ASYNC_WEB
        AsyncWebServerResponse *response = request->beginResponse(300, "text/plain", "");
        response->addHeader("Location", String("http://") + WiFi.softAPIP().toString());
        request->send(response);

#else
        REQ_OBJECT.sendHeader("Location", String("http://") +
                              toStrIp(REQ_OBJECT.client().localIP()), true);
        REQ_OBJECT.send(302, "text/plain", "");
        delay(100);
        REQ_OBJECT.client().stop();
#endif
        return true;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////////////////////
void espxxsrv_t::handleConfig(RQ_TYPE_AND_PARAM)
{
#if ASYNC_WEB
    WebPage = REQ_OBJECT.url();
#else
    WebPage = REQ_OBJECT.uri();
#endif
    if(REQ_OBJECT.hasArg("reset"))
    {
        app_t::TheApp->reset_graph();
    }
    if(REQ_OBJECT.hasArg("ramm"))
    {
        memset(&Ramm,0,sizeof(Ramm));
        hot_store();
    }
    if(REQ_OBJECT.hasArg("hours"))
    {
        app_t::TheApp->reset_hours();
    }
    if(REQ_OBJECT.hasArg("wificred"))
    {
        eeprom_t re(1);
        re->passwd[0]=0;
        re->ip[0]=0;
        re->ssid[0]=0;
        re.save();
        app_t::TheApp->reboot_board();
    }
    _faren = CFG(faren);
    String page = espxxsrv_t::start_htm();
    page += "<h1 class='header'>CONFIG</h1>";
    espxxsrv_t::hot=true;

    //page += F("<li>schedule     <input type='text' name='schedule' value='")+ _S(CFG(schedule)) + F("' >");
    //page += F("<li>autoenabled  <input type='text' name='autoenabled' value='")+ _S(CFG(autoenabled)) + F("' >");
    page += F("<form method='POST' action='configsave'>");
    /////////////// form //////////
    page += F("<table><tr><th>");
    page += F("Hours Time Zone</th><td><input type='text' value='");
    page += String(CFG(ntp_offset));
    page +=F("' name='tz'/></td></tr>");
    page += F("<tr><th>Keep Access Point ON (0/1)</th><td><input type='text' name='hsta' value='");
    page += _S(CFG(hsta));       page += F("' ></td></tr>");
    page += F("<tr><th>Access Point Name  </th><td><input type='text' readonly name='mcteck ' value='");
    page += CFG(mcteck);       page += F("' >");
    page += F("<tr><th>Access Point Password</th><td><input type='text' name='mtkpsw' value='");
    page += CFG(mtkpsw);  page += F("' >");
    String fh = CFG(faren) ? F("checked") : F("");
    page += F("<tr><th>Temperature In Fahrenheit </th><td><input type='checkbox' name='faren' ") + fh +F(" ></td></tr>");
    page += F("<tr><th>Temp Trigger</th><td><input type='text' name='trg' value='")+ _S(CFG(trg)) + F("' ></td></tr>");
    page += F("<rt><th>Hum Trigger</th><td><input type='text' name='hrg' value='")+ _S(CFG(hrg)) + F("' ></td></tr>");
    page += F("<tr><th>Relay Logic</th><td>\n<select style='width:220px' name='trl'>\n");
    page += F("<option value=0>"); page += get_logic(0);  page +=F("</option>\n");
    page += F("<option value=1 "); page += CFG(trigger_rule)==1?F("selected >"):F(">"); page+=get_logic(1); page+=F("</option>\n");
    page += F("<option value=2 "); page += CFG(trigger_rule)==2?F("selected >"):F(">"); page+=get_logic(2); page+=F("</option>\n");
    page += F("<option value=3 "); page += CFG(trigger_rule)==3?F("selected >"):F(">"); page+=get_logic(3); page+=F("</option>\n");
    page += F("<option value=4 "); page += CFG(trigger_rule)==4?F("selected >"):F(">"); page+=get_logic(4); page+=F("</option>\n");
    page += F("<option value=5 "); page += CFG(trigger_rule)==5?F("selected >"):F(">"); page+=get_logic(5); page+=F("</option>\n");
    page += F("<option value=6 "); page += CFG(trigger_rule)==6?F("selected >"):F(">"); page+=get_logic(6); page+=F("</option>\n");
    page += F("<option value=7 "); page += CFG(trigger_rule)==7?F("selected >"):F(">"); page+=get_logic(7); page+=F("</option>\n");
    page += F("<option value=8 "); page += CFG(trigger_rule)==8?F("selected >"):F(">"); page+=get_logic(8); page+=F("</option>\n");
    page += F("</select><fieldset>");
    for(int i=0;i<MAX_SENS;i++)
    {
        const sensor_th_t* ps = app_t::TheApp->sensors().sensor(i);
        if(ps)
        {
            if(ps->id()==CFG(sid))
                page += "*";
            page += F("<input type='radio' value='"); page+=ps->name(); page+=F("' name='i_"); page+= ps->id(); page+=F("' />");
            page += ps->name();
            page += ps->t_h();
            page+=F("</br>");
        }
    }
    page+=F("</fieldset></td></tr>\n");
    /*
    page += F("<li>h_host       <input type='text' name='h_host ' value='")+ _S(CFG(h_host)) + F("' >");
    page += F("<li>h_put        <input type='text' name='h_put ' value='")+ _S(CFG(h_put)) + F("' >");
    page += F("<li>h_get        <input type='text' name='h_get ' value='")+ _S(CFG(h_get)) + F("' >");
    page += F("<li>h_finger     <input type='text' name='h_finger ' value='")+ _S(CFG(h_finger)) + F("' >");
    page += F("<li>h_port       <input type='text' name='h_port ' value='")+ _S(CFG(h_port)) + F("' >");
    page += F("<li>mq_ttbroker  <input type='text' name='mq_ttbroker ' value='")+ _S(CFG(mq_ttbroker)) + F("' >");
    page += F("<li>mq_topic     <input type='text' name='mq_topic ' value='")+ _S(CFG(mq_topic)) + F("' >");
    page += F("<li>mq_user      <input type='text' name='mq_user ' value='")+ _S(CFG(mq_user)) + F("' >");
    page += F("<li>mq_pass      <input type='text' name='mq_pass ' value='")+ _S(CFG(mq_pass)) + F("' >");
    page += F("<li>mq_port      <input type='text' name='mq_port ' value='")+ _S(CFG(mq_port)) + F("' >");
    page += F("<li>get_interval <input type='text' name='get_interval' value='")+ _S(CFG(get_interval)) + F("' >");
    page += F("<li>ntp_offset   <input type='text' name='ntp_offset ' value='")+ _S(CFG(ntp_offset)) + F("' >");
    page += F("<li>ntp_srv      <input type='text' name='ntp_srv ' value='")+ _S(CFG(ntp_srv)) + F("' >");
    page += F("<li>ip           <input type='text' name='ip ' value='")+ _S(CFG(ip)) + F("' >");
    page += F("<li>gw           <input type='text' name='gw ' value='")+ _S(CFG(gw)) + F("' >");
    page += F("<li>subnet       <input type='text' name='subnet ' value='")+ _S(CFG(subnet)) + F("' >");
    page += F("<li>dnsa         <input type='text' name='dnsa ' value='")+ _S(CFG(dnsa)) + F("' >");
    page += F("<li>dnsb         <input type='text' name='dnsb ' value='")+ _S(CFG(dnsb)) + F("' >");
*/
    page += F("<tr><th colspan='2'><input type='submit'  style='float:right' name='apply' value='Apply'/></th>"
              "</tr><table>");
    page += F("</form>");
    page += F("<p class='centered'>");
    page += F("<li>Reboots: ");         page += String(app_t::TheApp->_ae._reboots);
    page += F("<li>Reboot reason: ");   page += String(app_t::TheApp->_ae._reason);
    page += F("<li>Reboot time: ");     page += Sclk._resettitme;
    page += F("<li>Up time: ");         page += String(millis()/(1000*60)); page += F(" mins");
    page += F("<li>Flash Writes: ");    page += String(app_t::TheApp->_ae._flash_writes);
    page += F("<li>EEprom Writes: ");   page += String(app_t::TheApp->_ae._eprom_writes);
    char et[64];
    ::sprintf(et,"N:%lu / L:%lu",Sclk.ntp_epoch_secs(),Sclk.getEpochTime());
    page += F("<li>Epoch time: ");      page += String(et);
    page += F("<li>Reset Graph: <a href=/config?reset=1>Click to reset</a>");
    page += F("<li>Reset Hours: <a href=/config?hours=1>Click to reset</a>");
    page += F("<li>Reset Errors: <a href=/config?ramm=1>Click to reset</a>");
    page += F("<li>Reset WIFI Login: <a href=/config?wificred=1'>Click to reset</a>");
    page += F("<li>(AP Only)Set Device Time from PC: <button ");
    page += Sclk.update_ntp(true) ? F("disabled") : F("");
    page += F(" onclick=\"set_time()\" >Set</button>  Curreent device time:");
    page += F("<i id='dev_time'>");
    page += Sclk.get_time();
    page += F("</i>");


    page += F("<li>Intervals:");
    for(int i=0;i<SAMPLES_PER_DAY;i++){
        page+=String(app_t::TheApp->_intervals[i]);
        page+=",";
        if((i+1)%32==0)page+="<br>";
    }

    page += F("</p>");
#if ASYNC_WEB
    espxxsrv_t::end_htm(page, true, RQ_PARAM);
#else
    espxxsrv_t::end_htm(page, true);
#endif

    //REQ_OBJECT.send(200, "text/html", page);
}

//////////////////////////////////////////////////////////////////////////////////////////
void espxxsrv_t::handleConfigSave(RQ_TYPE_AND_PARAM)
{
#if ASYNC_WEB
    WebPage = REQ_OBJECT.url();
#else
    WebPage = REQ_OBJECT.uri();
#endif
    if(!espxxsrv_t::hot)
    {
#if ASYNC_WEB
        handleRoot(request);
#else
        handleRoot();
#endif
        return;
    }
    LOG("/config save page");
    do{
        eeprom_t e(1);

        char hsta[16];
        char trg[16];
        char hrg[16];
        char trl[16];
        char mcteck[16];
        char mtkpsw[16];
        char tz[16];

#if ASYNC_WEB
        request->arg("hsta").toCharArray(hsta, sizeof(hsta) - 1);
        request->arg("trg").toCharArray(trg, sizeof(trg) - 1);
        request->arg("hrg").toCharArray(hrg, sizeof(hrg) - 1);
        request->arg("trl").toCharArray(trl, sizeof(trl) - 1);
        request->arg("mcteck").toCharArray(mcteck, sizeof(mcteck) - 1);
        request->arg("mtkpsw").toCharArray(mtkpsw, sizeof(mtkpsw) - 1);
        request->arg("tz").toCharArray(tz, sizeof(tz) - 1);
        e->faren = request->hasArg("faren");
#else
        REQ_OBJECT.arg("hsta").toCharArray(hsta, sizeof(hsta) - 1);
        REQ_OBJECT.arg("trg").toCharArray(trg, sizeof(trg) - 1);
        REQ_OBJECT.arg("hrg").toCharArray(hrg, sizeof(hrg) - 1);
        REQ_OBJECT.arg("trl").toCharArray(trl, sizeof(trl) - 1);
        REQ_OBJECT.arg("mcteck").toCharArray(mcteck, sizeof(mcteck) - 1);
        REQ_OBJECT.arg("mtkpsw").toCharArray(mtkpsw, sizeof(mtkpsw) - 1);
        REQ_OBJECT.arg("tz").toCharArray(tz, sizeof(tz) - 1);
        e->faren = REQ_OBJECT.hasArg("faren");
#endif
        e->hsta=::atoi(hsta);
        e->trg=::atoi(trg);
        e->hrg=::atoi(hrg);
        e->trigger_rule=(char)::atoi(trl);
        e->ntp_offset = ::atoi(tz);
        if(mcteck[0])::strcpy(CFG(mcteck),mcteck);
        if(mtkpsw[0])::strcpy(CFG(mtkpsw),mtkpsw);
        app_t::TheApp->_ae._eprom_writes++;

        for(int i=0;i<MAX_SENS;i++)
        {
            String key = "i_"; key+=String(i);
            if(REQ_OBJECT.hasArg(key.c_str()))
            {
                e->sid = i;
                break;
            }
        }
        e.save();
    }while(0);
    if(_faren != CFG(faren))
    {
        app_t::TheApp->convert_graph(_faren, CFG(faren));
    }
    app_t::TheApp->save();
    app_t::TheApp->reboot_board();
}

//////////////////////////////////////////////////////////////////////////////////////////
void espxxsrv_t::handleUser(RQ_TYPE_AND_PARAM)
{
#if ASYNC_WEB
    WebPage = REQ_OBJECT.url();
#else
    WebPage = REQ_OBJECT.uri();
#endif

    String page;
#if ASYNC_WEB
    app_t::TheApp->http_get(RQ_PARAM, page);
#else
    app_t::TheApp->http_get(&WifiSrv->_esp, page);
#endif
    REQ_OBJECT.send(200, "text/html", page);
}

//////////////////////////////////////////////////////////////////////////////////////////
void espxxsrv_t::handleGraph(RQ_TYPE_AND_PARAM)
{
    LOG(__FUNCTION__);
#if ASYNC_WEB
    WebPage = REQ_OBJECT.url();
#else
    WebPage = REQ_OBJECT.uri();
#endif
    // send all canvas
#if WITH_GRAPH
    int sens = 0;

    if(REQ_OBJECT.hasArg("sens"))
    {
        LOG("SENS=%d",REQ_OBJECT.arg("sens").toInt());
        sens = REQ_OBJECT.arg("sens").toInt();
    }
    REQ_OBJECT.send(200, "text/html", app_t::TheApp->_canvas.samples(sens));
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////
void espxxsrv_t::handleTime(RQ_TYPE_AND_PARAM)
{
#if ASYNC_WEB
    WebPage = REQ_OBJECT.url();
#else
    WebPage = REQ_OBJECT.uri();
#endif

    char timp[16] = {0};
    REQ_OBJECT.arg("time").toCharArray(timp,14);
    LOG("WT=%d", timp);
    sensdata_t at(eTIME, ::atoi(timp));
    app_t::TheApp->on_web_event(&at);
}

/////////////////////////////////////////////////////////////////////////////////////////
void espxxsrv_t::handleOta(RQ_TYPE_AND_PARAM)
{
    LOG(__FUNCTION__);
#if ASYNC_WEB
    WebPage = REQ_OBJECT.url();
#else
    WebPage = REQ_OBJECT.uri();
#endif
    String page = start_htm();
    page += "<h1 class='header'>UPDATES</h1>";
    page += "<div id='msg'></div><hr /><br /><br /><br />"
            "<form method='POST' id='form' name='form' "
            "action='/fileup' enctype='multipart/form-data' id='upload_form'>"
            "<table><tr><td><label class='but'><input type='file' style='background:black;' name='update'>Select new firmware...</label>"
            "</td><td><input type='submit'  value='Update' "
            "onclick=\"document.getElementById('m').innerHTML='Do not refresh! Please wait!'\"></td></tr></table>"
            "</form><br><div id='m'></div>";
    "<script>\n"
    "function redir() {\n"
    "window.location.href ='/';\n"
    "}\n"
    "function ls(event) {\n"
    "setTimeout(redir, 2000) ;\n"
    "}\n"
    "const form=document.getElementById('form');\n"
    "form.addEventListener('submit', ls);\n"
    "</script>\n";

#if ASYNC_WEB
    espxxsrv_t::end_htm(page, true, RQ_PARAM);
#else
    espxxsrv_t::end_htm(page, true);
#endif
    //REQ_OBJECT.send(200, "text/html", page);
}

/////////////////////////////////////////////////////////////////////////////////////////
void espxxsrv_t::handleWifi(RQ_TYPE_AND_PARAM)
{
#if ASYNC_WEB
    WebPage = REQ_OBJECT.url();
#else
    WebPage = REQ_OBJECT.uri();
#endif

    eeprom_t    e;
    String      page = espxxsrv_t::WifiSrv->start_htm();
    IPAddress   sta; sta.fromString("10.5.5.1");

    page += "<h1 class='header'>WIFI AND NETWORK</h1>";
    espxxsrv_t::hot=true;
#if !ASYNC_WEB
    if (REQ_OBJECT.client().localIP() == sta)
#else
    if (WiFi.localIP() == sta)
#endif
    {
        page += String(F("<p>Connected toSoft AP: "));
        page += CFG(mcteck);
        page += F("<p>");
    } else {
        page += String(F("<p>Connected to Wifi network: ")) + e->ip + F("</p>");
    }
    String inet = espxxsrv_t::ping(CFG(ntp_srv)) ? F(" Okay") : F(" Fail");
    String rout = espxxsrv_t::ping(CFG(gw)) ? F(" Okay") : F(" Fail");
    page +=
            String(F(
                       "\r\n<br />"
                       "<table><tr><th align='left'>SoftAP config</th></tr>"
                       "<tr><td>AP ")) + CFG(mcteck) +
            F("</td></tr>"
              "<tr><td>IP ") +
            toStrIp(WiFi.softAPIP()) +
            F("</td></tr>"
              "</table>"
              "\r\n<br />"
              "<table><tr><th align='left'>WLAN config</th></tr>"

              "<tr><td>SSID ") +
            String(e->ssid) +
            F("</td></tr>"
              "<tr><td>Can reach internet ") +
            inet+
            F("</td></tr>"

              "<tr><td>IP ") +
            toStrIp(WiFi.localIP()) +
            F("</td></tr><tr><td>Time Zone:")+
            String(e->ntp_offset)+
            F("</td></tr></table>"
              "\r\n<br />"
              "<table><tr><th align='left' colspan='2'>Found WIFI's (Do not connect to any WIFI below 65 dB. Press F5 if WIFI are missing)</th></tr>");
    Serial.println("scan start");
    int n = WiFi.scanNetworks();
    Serial.println("scan done");
    if (n > 0) {
        for (int i = 0; i < n; i++) {
            page += String(F("\r\n<tr><td><b>")) + WiFi.SSID(i) +
                    ((WiFi.encryptionType(i) == ENC_TYPE_NONE) ? F("</b> ") : F("</b> (SECURED) ")) + F("</td><td> (") +
                    WiFi.RSSI(i) + F(")</td></tr>");
        }
    } else {
        page += F("<tr><td>No </td><td> WFIFI's</td></tr>");
    }
    page += F("</table>"
              "<form method='POST' action='wifisave'>"
              "<table><tr><td colspan='2'><h4>Local WIFI Network</h4></td<tr>");
    page += F("<tr><th>SSID</th><td><input type='text' value='");
    page += CFG(ssid);
    page += F("' name='n'/></td></tr>");
    page += F("<tr><th> PASSWORD</th><td><input type='text' value='");
    page += CFG(passwd);
    page +=F("' name='p'/></td></tr>");
    page += F("<tr><th>STATIC IP</th><td><input type='text' placeholder='192.168.1.199' value='");
    page += CFG(ip);
    page +=F("' name='i'/></td></tr>");
    page += F("<tr><th>GATEWAY</th><td><input type='text' placeholder='192.168.1.0' value='");
    page += CFG(gw);
    page +=F("' name='gw'/></td></tr>");
    page += F("<tr><th>SUBNET</th><td><input type='text' placeholder='255.255.255.0' value='");
    page += CFG(mask);
    page +=F("' name='s'/></td></tr>");
    page += F("<tr><th>DNS SERVER 1</th><td><input type='text' value='");
    page += CFG(dnsa);
    page +=F("' name='d1'/></td></tr>");
    page += F("<tr><th>DNS SERVER 2</th><td><input type='text' value='");
    page += CFG(dnsb);
    page +=F("' name='d2'/></td></tr>");
    page += F("<tr><th>NTP SERVER:(blank to disable) </th><td><input type='text' value='");
    page += CFG(ntp_srv);
    page +=F("' name='nt'/></td></tr>");
    page += F("<tr><th>TIME ZONE HOURS</th><td><input type='text' value='");
    page += String(CFG(ntp_offset));
    page +=F("' name='z'/></td></tr>");
    page += F("<tr><td colspan='2'><input type='submit'  "
              " style='float:right' value='Apply'/></td></tr></table></form>");


#if ASYNC_WEB
    espxxsrv_t::end_htm(page, true, RQ_PARAM);
#else
    espxxsrv_t::end_htm(page, true);
#endif

}

void espxxsrv_t::handleWifiSave(RQ_TYPE_AND_PARAM)
{
#if ASYNC_WEB
    WebPage = REQ_OBJECT.url();
#else
    WebPage = REQ_OBJECT.uri();
#endif
    LOG(__FUNCTION__);
    if(!espxxsrv_t::hot)
    {
        handleRoot(RQ_PARAM);
        return;
    }

    eeprom_t e(1);

    char ssid[32]={0};
    char passwd[32]={0};
    char ip[24]={0};
    char gw[24]={0};
    char s[24]={0};
    char d1[24]={0};
    char d2[24]={0};
    char nt[24]={0};
    char time[24]={0};

    REQ_OBJECT.arg("n").toCharArray(ssid, sizeof(ssid) - 1);
    REQ_OBJECT.arg("p").toCharArray(passwd, sizeof(passwd) - 1);
    REQ_OBJECT.arg("i").toCharArray(ip, sizeof(ip) - 1);
    REQ_OBJECT.arg("gw").toCharArray(gw, sizeof(gw) - 1);
    REQ_OBJECT.arg("d1").toCharArray(d1, sizeof(d1) - 1);
    REQ_OBJECT.arg("d2").toCharArray(d2, sizeof(d2) - 1);
    REQ_OBJECT.arg("nt").toCharArray(nt, sizeof(nt) - 1);
    REQ_OBJECT.arg("z").toCharArray(time, sizeof(time) - 1);

    if(ssid[0])     ::strcpy(e->ssid, ssid); else e->ssid[0]=0;
    if(passwd[0])   ::strcpy(e->passwd, passwd); else e->passwd[0]=0;
    if(ip[0])       ::strcpy(e->ip, ip); else e->ip[0]=0;
    if(gw[0])       ::strcpy(e->gw, gw);
    if(s[0])        ::strcpy(e->mask,s);
    if(d1[0])       ::strcpy(e->dnsa, d1);
    if(d2[0])       ::strcpy(e->dnsb, d2);
    if(nt[0])       ::strcpy(e->ntp_srv, nt); else e->ntp_srv[0]=0;
    if(time[0])     e->ntp_offset = ::atoi(time);

    LOG(" %s %s %s %s %d", e->ssid, e->passwd, e->ip, e->gw, e->ntp_offset);
    e.save();
    delay(200);
    e.load();
    String page = F("<li>SAVING<ul>");
    page += F("<li> SSID ")    + _S(CFG(ssid));
    page += F("<li> IP ")      + _S(CFG(ip));
    page += F("<li> GW ")      + _S(CFG(gw));
    page += F("<li> SN ")      + _S(CFG(mask));
    page += F("<li> DN1 ")     + _S(CFG(dnsa));
    page += F("<li> DN2 ")     + _S(CFG(dnsb));
    page += F("<li> NTP ")     + _S(CFG(ntp_srv));
    page += F("<li> ZONE ")    + _S(CFG(ntp_offset));
    page += F("<li>Wait 45 seconds, Connect to your router and click: ");
    page += F("<a href='http://") + String(e->ip) + F("'>mcteck.local</a></ul>");
#if ASYNC_WEB
    espxxsrv_t::end_htm(page, true, RQ_PARAM);
#else
    espxxsrv_t::end_htm(page, true);
#endif
    app_t::TheApp->_ae._eprom_writes++;
    delay(512);
    e.save();
    app_t::TheApp->save();
    app_t::TheApp->reboot_board(0, &page);
}

void espxxsrv_t::handleRoot(RQ_TYPE_AND_PARAM)
{
#if ASYNC_WEB
    WebPage = REQ_OBJECT.url();
#else
    WebPage = REQ_OBJECT.uri();
#endif

    LOG("HANDLE ROOT %s ", WebPage);
#if ASYNC_WEB
    if(espxxsrv_t::WifiSrv->sta_connected()==false)
    {
        if (espxxsrv_t::WifiSrv->_capturePage(request))
        {
            return;
        }
    }
#else
    if(espxxsrv_t::WifiSrv->sta_connected()==false)
    {
        if (espxxsrv_t::WifiSrv->_capturePage())
        {
            return;
        }
    }
#endif
    String page;
    app_t::TheApp->http_get(&REQ_OBJECT,page);
    REQ_OBJECT.send(200, "text/html", page);
}

#if ASYNC_WEB
void espxxsrv_t::end_htm(String & page, bool endht, RQ_TYPE_AND_PARAM)
#else
void espxxsrv_t::end_htm(String & page, bool endht)
#endif

{
    LOG(__FUNCTION__);
    if(endht){
        page += F("</div></body></html>");
    }
    REQ_OBJECT.send(200, "text/html", page);

#if !ASYNC_WEB
    espxxsrv_t::WifiSrv->flush();
#endif
    delay(8);
}

void espxxsrv_t::handleJs(RQ_TYPE_AND_PARAM)
{
    if(WebPage=="/config")
    {
        LOG(__FUNCTION__);
        REQ_OBJECT.send(200, "text/javascript", get_js());
    }
    else
        REQ_OBJECT.send(200, "text/javascript", "function f(){;}");
}

void espxxsrv_t::handleCss(RQ_TYPE_AND_PARAM)
{
    LOG(__FUNCTION__);
    REQ_OBJECT.send(200, "text/css", get_css());
}

const String& espxxsrv_t::start_htm(bool content)
{
    static String ret;
    LOG(__FUNCTION__);
    ret = "";
    // REQ_OBJECT.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    // REQ_OBJECT.sendHeader("Pragma", "no-cache");
    // REQ_OBJECT.sendHeader("Expires", "-1");
    if(content)
    {
        if(GlobalError)
        {
            ret = F("<!DOCTYPE html><html lang='en'>\n<head>\n"
                    "<meta name='viewport' content='width=device-width'>\n"
                    "<title>MARIUTEK</title>\n"
                    "<meta http-equiv='Cache-control' content='public'>"
                    "</head>\n<body>\n"
                    "<nav><ul class='menu'>\n"
                    "<li>RUN ERRORS. UPDATE\n"
                    "<li> <a href='/ota'>DO UPDATE</a>\n"
                    "<li> <a href='?clean'>TRY REBOOT</a>\n"
                    "</ul></nav><div class='container'>\n");
        }
        else
        {
            ret = F("<!DOCTYPE html><html lang='en'>\n<head>\n"
                    "<meta name='viewport' content='width=device-width'>\n"
                    "<meta http-equiv='Cache-control' content='public'>"
                    "<title>MCTECK</title>\n"
                    "<link rel='stylesheet' href='/cssp'>\n"
                    "<script type='module' src='/js'></script>\n"
                    "</head>\n<body>\n"
                    "<nav><ul class='menu'>\n"
                    "<li> <a href='/'>HOME</a></a>\n"
                    "<li> <a href='/wifi'>WIFI</a>\n"
                    "<li> <a href='/ota'>UPDATE</a>\n"
                    "<li> <a href='/config'>CONFIG</a>\n"
                    "</ul></nav><div class='container'>\n");
        }
    }
    return ret;
}


bool espxxsrv_t::_isIp(const String& str)
{
    for (size_t i = 0; i < str.length(); i++) {
        int c = str.charAt(i);
        if (c !='.' && (c < '0' || c > '9')) {
            return false;
        }
    }
    return true;
}

String espxxsrv_t::toStrIp(const IPAddress& ip)
{
    String res = "";
    for (int i = 0; i < 3; i++) {
        res += String((ip >> (8 * i)) & 0xFF) + ".";
    }
    res += String(((ip >> 8 * 3)) & 0xFF);
    return res;
}

void espxxsrv_t::handleFetch(RQ_TYPE_AND_PARAM)
{
    espxxsrv_t::WifiSrv->_handleFetch(RQ_PARAM);
}

void espxxsrv_t::handleError(RQ_TYPE_AND_PARAM)
{
    REQ_OBJECT.send(200, "text/html", "Error OTA");
}

void espxxsrv_t::handleSensor(RQ_TYPE_AND_PARAM)
{
#if I2C_SDA
    String pa = start_htm();
    LOG(__FUNCTION__);
    if(REQ_OBJECT.hasArg("sens"))
    {
        sensor_th_t* p = (sens_sht21*)app_t::TheApp->_sensors.sensor(REQ_OBJECT.arg("sens").c_str());
        if(p)
        {
            pa+= F("<script>function bk(){alert('"); pa+=p->name(); pa+=F("');}</script>");
            pa + F("<li>4 points Temp corrections<input type='text' id='ths' value='0,0,0,0'> <button onclick='bk()'>Apply </button>"
            "<li>4 points Humidity corrections<input type='text' id='hhs' value='0,0,0,0'> <button onclick='bk()'>Apply </button>");
        }
        else
        {
            pa += REQ_OBJECT.arg("sens");
        }
    }
    else
    {
        pa += F("This sensor has no settings");
    }

#if ASYNC_WEB
    espxxsrv_t::end_htm(pa, true, RQ_PARAM);
#else
    espxxsrv_t::end_htm(pa, true);
#endif

#endif
}
