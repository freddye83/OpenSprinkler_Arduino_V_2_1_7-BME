/* ==========================================================================================================
This is a fork of Rays OpenSprinkler code thats amended to use alternative hardware:

EtherCardW5100.h and EtherCardW5100.cpp implements a minimal set of functions
as a wrapper to replace the ENC28J60 EtherCard class libraries with the standard
Arduino Wiznet5100 Ethernet library.

Version:     Opensprinkler 2.1.8

Date:        May 2018

Repository:  https://github.com/plainolddave/OpenSprinkler-Arduino

License:     Creative Commons Attribution-ShareAlike 3.0 license

Refer to the README file for more information

========================================================================================================== */

#include <stdarg.h>
#include <avr/eeprom.h>
#include "EtherCardW5100.h"

//================================================================
// Utility functions
//================================================================

/// <summary>
/// Copy four bytes to an IPAddress object
/// </summary>
/// <param name="src">Pointer to the 4 byte source</param>
/// <returns></returns>
IPAddress Byte2IP(const byte *src)
{
    return IPAddress(src[0], src[1], src[2], src[3]);
}

/// <summary>
/// Copy an IPAddress object to four bytes
/// </summary>
/// <param name="src">Pointer to the 4 byte source</param>
/// <returns></returns>
void IP2Byte(IPAddress ip, byte *dest)
{
    dest[0] = ip[0];
    dest[1] = ip[1];
    dest[2] = ip[2];
    dest[3] = ip[3];
}

/// <summary>
/// webutil.cpp - convert a single hex digit character to its integer value
/// </summary>
/// <param name="c">single character</param>
/// <returns>integer value</returns>
unsigned char EtherCardW5100::h2int(char c)
{
    if (c >= '0' && c <= '9')
    {
        return ((unsigned char)c - '0');
    }
    if (c >= 'a' && c <= 'f')
    {
        return ((unsigned char)c - 'a' + 10);
    }
    if (c >= 'A' && c <= 'F')
    {
        return ((unsigned char)c - 'A' + 10);
    }
    return (0);
}

/// <summary>
/// webutil.cpp - convert a single character to a 2 digit hex str
/// </summary>
/// <param name="c">single char</param>
/// <param name="hstr">2 digit hex string with terminating '\0'</param>
void EtherCardW5100::int2h(char c, char *hstr)
{
    hstr[1] = (c & 0xf) + '0';
    if ((c & 0xf) > 9)
    {
        hstr[1] = (c & 0xf) - 10 + 'a';
    }
    c = (c >> 4) & 0xf;
    hstr[0] = c + '0';
    if (c > 9)
    {
        hstr[0] = c - 10 + 'a';
    }
    hstr[2] = '\0';
}

EtherCardW5100 etherW5100;

// Declare static data members (ethercard.cpp)
uint8_t EtherCardW5100::mymac[6];				// MAC address
uint8_t EtherCardW5100::myip[4];				// IP address
uint8_t EtherCardW5100::netmask[4];				// Netmask
uint8_t EtherCardW5100::gwip[4];				// Gateway
uint8_t EtherCardW5100::dhcpip[4];				// DHCP server IP address
uint8_t EtherCardW5100::dnsip[4];				// DNS server IP address
uint8_t EtherCardW5100::hisip[4];				// DNS lookup result
uint16_t EtherCardW5100::hisport = 80;			// TCP port to connect to (default 80)
bool EtherCardW5100::using_dhcp;				// True if using DHCP
uint8_t EtherCardW5100::broadcastip[4];		// Subnet broadcast address
bool EtherCardW5100::persist_tcp_connection; // False to break connections on first packet received
uint16_t EtherCardW5100::delaycnt = 0;		// Counts number of cycles of packetLoop when no packet received - used to trigger periodic gateway ARP request

// Declare static data members from enc28j60.h
uint16_t EtherCardW5100::bufferSize;

// Declare static data members for this wrapper class
IPAddress EtherCardW5100::ntpip;
SOCKET EtherCardW5100::ping_socket = 0;
ICMPPing EtherCardW5100::ping(ping_socket, 1);
ICMPEchoReply EtherCardW5100::ping_result;
EthernetServer EtherCardW5100::incoming_server(hisport);
EthernetClient EtherCardW5100::incoming_client;
EthernetClient EtherCardW5100::outgoing_client;
EthernetUDP EtherCardW5100::udp_client;
DNSClient EtherCardW5100::dns_client;

// Declare static data (from tcpip.cpp)
static uint8_t www_fd = 0;						// ID of current http request (only one http request at a time - one of the 8 possible concurrent TCP/IP connections)
static const char *client_urlbuf;				// Pointer to c-string path part of HTTP request URL
static const char *client_urlbuf_var;			// Pointer to c-string filename part of HTTP request URL
static const char *client_hoststr;				// Pointer to c-string hostname of current HTTP request
static const char *client_additionalheaderline;	// Pointer to c-string additional http request header info
static const char *client_postval;
static tcpstate_t outgoing_client_state;		//TCP connection state: 1=Send SYN, 2=SYN sent awaiting SYN+ACK, 3=Established, 4=Not used, 5=Closing, 6=Closed
static void(*client_browser_cb) (uint8_t, uint16_t, uint16_t);	// Pointer to callback function to handle result of current HTTP request

// External variables defined in main .ino file
extern BufferFiller bfill;

//=================================================================================
// Ethercard Wrapper Functions
//=================================================================================

/// <summary>
/// Initialise the network interface
/// </summary>
/// <param name="size">Size of data buffer (not used)</param>
/// <param name="macaddr">Hardware address to assign to the network interface (6 bytes) (not used)</param>
/// <param name="csPin">Arduino pin number connected to chip select. Default = 8</param>
/// <returns>Firmware version or zero on failure.</returns>
uint8_t EtherCardW5100::begin(const uint16_t size, const uint8_t* macaddr, uint8_t csPin)
{
    bufferSize = size;
    using_dhcp = false;
    copyMac(mymac, macaddr);
    return 1; //0 means fail
}

/// <summary>
/// Configure network interface with static IP
/// </summary>
/// <param name="my_ip">IP address (4 bytes). 0 for no change.</param>
/// <param name="gw_ip">Gateway address (4 bytes). 0 for no change. Default = 0</param>
/// <param name="dns_ip">DNS address (4 bytes). 0 for no change. Default = 0</param>
/// <param name="mask">Subnet mask (4 bytes). 0 for no change. Default = 0</param>
/// <returns>Returns true on success - actually always true</returns>
bool EtherCardW5100::staticSetup(const uint8_t* my_ip, const uint8_t* gw_ip, const uint8_t* dns_ip, const uint8_t* mask)
{
    using_dhcp = false;

    // convert bytes to IPAddress
    IPAddress ip = Byte2IP(my_ip);
    IPAddress gw = Byte2IP(gw_ip);
    IPAddress dns = Byte2IP(dns_ip);
    IPAddress subnet = Byte2IP(mask);

    // initialize the ethernet device and start listening for clients
    Ethernet.begin(mymac, ip, dns, gw, subnet);
    incoming_server.begin();
    udp_client.begin(NTP_CLIENT_PORT);

    // save the values
    IP2Byte(Ethernet.localIP(), myip);
    IP2Byte(Ethernet.gatewayIP(), gwip);
    IP2Byte(Ethernet.dnsServerIP(), dnsip);
    IP2Byte(Ethernet.subnetMask(), netmask);

    return true;
}

/// <summary>
/// Configure network interface with DHCP
/// </summary>
/// <param name="hname">hostname (not implememted)</param>
/// <param name="fromRam">lookup from RAM</param>
/// <returns>True if DHCP successful</returns>
bool EtherCardW5100::dhcpSetup(const char *hname, bool fromRam)
{
    using_dhcp = true;

    /* Ignore the hostname - need to extend the standard Arduino ethernet library to implement this
    if ( hname != NULL )
    {
        if ( fromRam )
        {
            strncpy ( hostname, hname, DHCP_HOSTNAME_MAX_LEN );
        }
        else
        {
            strncpy_P ( hostname, hname, DHCP_HOSTNAME_MAX_LEN );
        }
    }
    else
    {
        // Set a unique hostname, use Arduino-?? with last octet of mac address
        hostname[8] = toAsciiHex ( mymac[5] >> 4 );
        hostname[9] = toAsciiHex ( mymac[5] );
    }
    */

    DEBUG_PRINT(F("Hostname:   "));
    // DEBUG_PRINT (hostname);
    DEBUG_PRINTLN(F("(not implemented)"));

    // initialize the ethernet device
    if (Ethernet.begin(mymac) == 0)
        return false;

    // start listening for clients
    incoming_server.begin();
    udp_client.begin(NTP_CLIENT_PORT);

    // save the values
    IP2Byte(Ethernet.localIP(), myip);
    IP2Byte(Ethernet.gatewayIP(), gwip);
    IP2Byte(Ethernet.dnsServerIP(), dnsip);
    IP2Byte(Ethernet.subnetMask(), netmask);

    return true;
}

/// <summary>
/// Parse received data
/// Note that data buffer is shared by receive and transmit functions
/// Only handles TCP (not UDP)
/// </summary>
/// <param name="plen">Size of data to parse(e.g. return value of packetReceive()).</param>
/// <returns>Offset of TCP payload data in data buffer or zero if packet processed</returns>
uint16_t EtherCardW5100::packetLoop(uint16_t plen)
{
    uint16_t len = 0;

    // remember that plen passed in from packetReceive is data length plus
    // a dummy TCP header (the rest of the TCP packet is stripped out)

    // nothing received
    if (plen == 0)
    {
        // Receive incoming TCP data for client
        if (outgoing_client_state == TCP_ESTABLISHED)
        {
            if (outgoing_client.available())
            {
                DEBUG_PRINT(F("Browse URL: client received: "));

                // set all bytes in the buffer to 0 - add a
                // byte to simulate space for the TCP header
                memset(buffer, 0, bufferSize);
                memset(buffer, ' ', TCP_OFFSET);
                len = TCP_OFFSET;

                while (outgoing_client.available() && (len < bufferSize))
                {
                    buffer[len] = outgoing_client.read();
                    DEBUG_PRINT(buffer[len]);
                    len++;
                }
                DEBUG_PRINTLN(F(""));

                // send data to the callback
                if (len > TCP_OFFSET)
                {
                    (*client_browser_cb) (0, TCP_OFFSET, len);
                }
                // if the server has disconnected, stop the client
                if (!outgoing_client.connected())
                {
                    DEBUG_PRINTLN(F("Browse URL: client disconnected"));
                    outgoing_client.stop();
                    outgoing_client_state = TCP_CLOSED;
                }
            }
        }

        // Check for renewal of DHCP lease
        if (using_dhcp)
            Ethernet.maintain();

        return 0;
    }
    else
    {
        //If we are here then this is a TCP/IP packet targetted at us and not
        // related to our client connection so accept treat it as a new request
        return TCP_OFFSET;
    }
}

/// <summary>
/// Send NTP request
/// </summary>
/// <param name="nt_pip">IP address of NTP server</param>
/// <param name="srcport">IP port to send from</param>
void EtherCardW5100::ntpRequest(uint8_t *ntp_ip, uint8_t srcport)
{
    // set all bytes in the buffer to 0
    memset(buffer, 0, bufferSize);

    // Initialize values needed to form NTP request
    buffer[0] = 0b11100011;    // LI, Version, Mode
    buffer[1] = 0;             // Stratum, or type of clock
    buffer[2] = 6;             // Polling Interval
    buffer[3] = 0xEC;          // Peer Clock Precision
    // 8 bytes of zero for Root Delay & Root Dispersion
    buffer[12] = 49;
    buffer[13] = 0x4E;
    buffer[14] = 49;
    buffer[15] = 52;

    // If a zero IP address is set, use a pool ntp server (this is more reliable)
    if (ntp_ip[0] == 0 && ntp_ip[1] == 0 && ntp_ip[2] == 0 && ntp_ip[3] == 0)
    {
        if (dnsLookup("pool.ntp.org", false))
            ntpip = IPAddress(hisip[0], hisip[1], hisip[2], hisip[3]);
        else
        {
            DEBUG_PRINT(F("NTP request failed - could not resolve IP"));
            return;
        }
    }
    else
        ntpip = IPAddress(ntp_ip[0], ntp_ip[1], ntp_ip[2], ntp_ip[3]);

    // all NTP fields have been given values, now you can send a packet requesting a timestamp:
    udp_client.beginPacket(ntpip, NTP_CLIENT_PORT);		// NTP requests are to port 123
    udp_client.write(buffer, NTP_PACKET_SIZE);
    udp_client.endPacket();

    DEBUG_PRINT(F("NTP request sent to "));
    printIp(ntp_ip);
    DEBUG_PRINT(F(": "));

#ifdef SERIAL_DEBUG
    for (uint8_t c = 0; c < 16; c++)
    {
        DEBUG_PRINTF(buffer[c], HEX);
        DEBUG_PRINT(F(" "));
    }
#endif
    DEBUG_PRINTLN("");
}

/// <summary>
/// Ethercard.cpp - Process network time protocol response
/// </summary>
/// <param name="time">Pointer to integer to hold result</param>
/// <param name="dstport_l">Destination port to expect response. Set to zero to accept on any port</param>
/// <returns>True (1) on success</returns>
byte EtherCardW5100::ntpProcessAnswer(uint32_t *time, byte dstport_l)
{
    int packetSize = udp_client.parsePacket();
    if (packetSize)
    {
        DEBUG_PRINT(F("NTP response received from "));
        DEBUG_PRINT(udp_client.remoteIP()[0]);
        DEBUG_PRINT(F("."));
        DEBUG_PRINT(udp_client.remoteIP()[1]);
        DEBUG_PRINT(F("."));
        DEBUG_PRINT(udp_client.remoteIP()[2]);
        DEBUG_PRINT(F("."));
        DEBUG_PRINT(udp_client.remoteIP()[3]);
        DEBUG_PRINT(F(":"));

        // check the packet is from the correct timeserver IP and port
        if (udp_client.remotePort() != 123 || udp_client.remoteIP() != ntpip)
        {
            DEBUG_PRINTLN(F(" (invalid IP or port)"));
            return 0;
        }

        //the timestamp starts at byte 40 of the received packet and is four bytes, or two words, long.
        udp_client.read(buffer, packetSize);
        ((byte*)time)[3] = buffer[40];
        ((byte*)time)[2] = buffer[41];
        ((byte*)time)[1] = buffer[42];
        ((byte*)time)[0] = buffer[43];

        DEBUG_PRINTLN((uint32_t)time);
        return 1;
    }
    return 0;
}

//=================================================================================
// Webutil.cpp
//=================================================================================

/// <summary>
/// webutil.cpp - copies an IP address
/// There is no check of source or destination size. Ensure both are 4 bytes
/// </summary>
/// <param name="dst">dst Pointer to the 4 byte destination</param>
/// <param name="src">src Pointer to the 4 byte source</param>
void EtherCardW5100::copyIp(byte *dst, const byte *src)
{
    memcpy(dst, src, 4);
}

/// <summary>
/// webutil.cpp - copies a hardware address
/// There is no check of source or destination size. Ensure both are 6 bytes
/// </summary>
/// <param name="dst">dst Pointer to the 6 byte destination</param>
/// <param name="src">src Pointer to the 6 byte destination</param>
void EtherCardW5100::copyMac(byte *dst, const byte *src)
{
    memcpy(dst, src, 6);
}

void EtherCardW5100::printIp(const char* msg, const uint8_t *buf)
{
    Serial.print(msg);
    EtherCardW5100::printIp(buf);
    Serial.println();
}

void EtherCardW5100::printIp(const __FlashStringHelper *ifsh, const uint8_t *buf)
{
    DEBUG_PRINT(ifsh);
    EtherCardW5100::printIp(buf);
    DEBUG_PRINTLN();
}

void EtherCardW5100::printIp(const uint8_t *buf)
{
    for (uint8_t i = 0; i < 4; ++i)
    {
        DEBUG_PRINTF(buf[i], DEC);
        if (i < 3)
            DEBUG_PRINT('.');
    }
}

/// <summary>
/// webutil.cpp - search for a string of the form key=value in a string that looks like q?xyz=abc&uvw=defgh HTTP/1.1\\r\\n
/// Ensure strbuf has memory allocated of at least maxlen + 1 (to accomodate result plus terminating null)
/// </summary>
/// <param name="str">Pointer to the null terminated string to search</param>
/// <param name="strbuf">Pointer to buffer to hold null terminated result string</param>
/// <param name="maxlen">Maximum length of result</param>
/// <param name="key">Pointer to null terminated string holding the key to search for</param>
/// <returns>Length of the value. 0 if not found</returns>
byte EtherCardW5100::findKeyVal(const char *str, char *strbuf, byte maxlen, const char *key)
{
    byte found = 0;
    byte i = 0;
    const char *kp;
    kp = key;
    while (*str &&  *str != ' ' && *str != '\n' && found == 0)
    {
        if (*str == *kp)
        {
            kp++;
            if (*kp == '\0')
            {
                str++;
                kp = key;
                if (*str == '=')
                {
                    found = 1;
                }
            }
        }
        else
        {
            kp = key;
        }
        str++;
    }
    if (found == 1)
    {
        // copy the value to a buffer and terminate it with '\0'
        while (*str &&  *str != ' ' && *str != '\n' && *str != '&' && i < maxlen - 1)
        {
            *strbuf = *str;
            i++;
            str++;
            strbuf++;
        }
        *strbuf = '\0';
    }
    // return the length of the value
    return (i);
}

/// <summary>
/// webutil.cpp - decode a URL string e.g "hello%20joe" or "hello+joe" becomes "hello joe"
/// </summary>
/// <param name="urlbuf">Pointer to the null terminated URL (urlbuf is modified)</param>
void EtherCardW5100::urlDecode(char *urlbuf)
{
    char c;
    char *dst = urlbuf;
    while ((c = *urlbuf) != 0)
    {
        if (c == '+') c = ' ';
        if (c == '%')
        {
            c = *++urlbuf;
            c = (h2int(c) << 4) | h2int(*++urlbuf);
        }
        *dst++ = c;
        urlbuf++;
    }
    *dst = '\0';
}

/// <summary>
/// webutil.cpp - encode a URL, replacing illegal charaters like ' '.
/// There must be enough space in urlbuf. In the worst case that is 3 times the length of str
/// </summary>
/// <param name="str">str Pointer to the null terminated string to encode</param>
/// <param name="urlbuf">urlbuf Pointer to a buffer to contain the null terminated encoded URL</param>
void EtherCardW5100::urlEncode(char *str, char *urlbuf)
{
    char c;
    while ((c = *str) != 0)
    {
        if (c == ' ' || isalnum(c))
        {
            if (c == ' ')
            {
                c = '+';
            }
            *urlbuf = c;
            str++;
            urlbuf++;
            continue;
        }
        *urlbuf = '%';
        urlbuf++;
        int2h(c, urlbuf);
        urlbuf++;
        urlbuf++;
        str++;
    }
    *urlbuf = '\0';
}

/// <summary>
/// webutil.cpp - Convert an IP address from dotted decimal formated string to 4 bytes
/// </summary>
/// <param name="bytestr">Pointer to the 4 byte array to store IP address</param>
/// <param name="str">Pointer to string to parse</param>
/// <returns>0 on success</returns>
byte EtherCardW5100::parseIp(byte *bytestr, char *str)
{
    char *sptr;
    byte i = 0;
    sptr = NULL;
    while (i < 4)
    {
        bytestr[i] = 0;
        i++;
    }
    i = 0;
    while (*str && i < 4)
    {
        // if a number then start
        if (sptr == NULL && isdigit(*str))
        {
            sptr = str;
        }
        if (*str == '.')
        {
            *str = '\0';
            bytestr[i] = (atoi(sptr) & 0xff);
            i++;
            sptr = NULL;
        }
        str++;
    }
    *str = '\0';
    if (i == 3)
    {
        bytestr[i] = (atoi(sptr) & 0xff);
        return (0);
    }
    return (1);
}

/// <summary>
/// webutil.cpp - take a byte string and convert it to a human readable display string
/// </summary>
/// <param name="resultstr">Pointer to a buffer to hold the resulting null terminated string</param>
/// <param name="bytestr">Pointer to the byte array containing the address to convert</param>
/// <param name="len">Length of the array (4 for IP address, 6 for hardware (MAC) address)</param>
/// <param name="separator">Delimiter character (typically '.' for IP address and ':' for hardware (MAC) address)</param>
/// <param name="base">Base for numerical representation (typically 10 for IP address and 16 for hardware (MAC) address</param>
void EtherCardW5100::makeNetStr(char *resultstr, byte *bytestr, byte len, char separator, byte base)
{
    byte i = 0;
    byte j = 0;
    while (i < len)
    {
        itoa((int)bytestr[i], &resultstr[j], base);
        // search end of str:
        while (resultstr[j])
        {
            j++;
        }
        resultstr[j] = separator;
        j++;
        i++;
    }
    j--;
    resultstr[j] = '\0';
}

//=================================================================================
// enc28j60.cpp
//=================================================================================

/// <summary>
/// enc28j60.cpp - copy recieved packets to data buffer
/// Data buffer is shared by recieve and transmit functions
/// </summary>
/// <returns>Size of recieved data packet</returns>
uint16_t EtherCardW5100::packetReceive()
{
    // listen for incoming clients
    // ** remember this client object is different from the W5100client used for tcp requests **
    incoming_client = incoming_server.available();

    if (incoming_client)
    {
        // set all bytes in the buffer to 0 - add a
        // byte to simulate space for the TCP header
        memset(buffer, 0, bufferSize);
        memset(buffer, ' ', TCP_OFFSET);
        uint16_t i = TCP_OFFSET; // add a space for TCP offset

        DEBUG_PRINT(F("Server request: "));

        while (incoming_client.connected() && (i < bufferSize))
        {
            if (incoming_client.available())
                buffer[i] = incoming_client.read();

            // print readable ascii characters
            if (buffer[i] >= 0x08 && buffer[i] <= 0x0D)
            {
                DEBUG_PRINT(F(" "));		// substitute a space so less rows in serial output
            }
            else if (buffer[i] > 0x1F)
            {
                DEBUG_PRINT((char)buffer[i]);
            }

            i++;
        }
        DEBUG_PRINTLN(F(""));
        return i;
    }
    else
        return 0;
}

//=================================================================================
// tcpip.cpp
//=================================================================================

/// <summary>
/// tcpip.cpp - send a response to a HTTP request
/// </summary>
/// <param name="dlen">Size of the HTTP (TCP) payload</param>
void EtherCardW5100::httpServerReply(word dlen)
{
    // ignore dlen - just add a null termination
    // to the buffer and print it out to the client
    buffer[bfill.position() + TCP_OFFSET] = '\0';
    incoming_client.print((char*)bfill.buffer());

    // close the connection:
    delay(1); // give the web browser time to receive the data
    incoming_client.stop();
}

/// <summary>
/// tcpip.cpp - send a response to a HTTP request
/// </summary>
/// <param name="dlen">Size of the HTTP (TCP) payload</param>
/// <param name="flags">TCP flags</param>
void EtherCardW5100::httpServerReply_with_flags(uint16_t dlen, uint8_t flags)
{
    // Need functionality to handle:
    //     - TCP_FLAGS_ACK_V
    //     - TCP_FLAGS_FIN_V
    //     - TCP_FLAGS_ACK_V|TCP_FLAGS_FIN_V

    // Same as above - ignore dlen & just add a null termination and print it out to the client
    buffer[bfill.position() + TCP_OFFSET] = '\0';
    incoming_client.print((char*)bfill.buffer());
    delay(1); // give the web browser time to receive the data

    if ((flags&TCP_FLAGS_FIN_V) != 0) // final packet in the stream
    {
        // close the connection:
        incoming_client.stop();
    }
}

/// <summary>
/// tcpip.cpp - acknowledge TCP message
/// </summary>
void EtherCardW5100::httpServerReplyAck()
{
    /*
    make_tcp_ack_from_any(info_data_len,0); // send ack for http get
    get_seq(); //get the sequence number of packets after an ack from GET
    */
}

/// <summary>
/// Populate the bufferfiller with www request header
/// </summary>
/// <param name="fd">File descriptor (always www_fd)</param>
/// <returns>current buffer filler position including TCP_OFFSET</returns>
uint16_t EtherCardW5100::www_client_internal_datafill_cb(uint8_t fd)
{
    bfill = EtherCardW5100::tcpOffset();

    if (fd == www_fd)
    {
        if (client_postval == 0)
        {
            bfill.emit_p(PSTR("GET $F$S HTTP/1.0\r\n"
                "Host: $F\r\n"
                "$F\r\n"
                "\r\n"), client_urlbuf,
                client_urlbuf_var,
                client_hoststr, client_additionalheaderline);
        }
        else
        {
            const char* ahl = client_additionalheaderline;
            bfill.emit_p(PSTR("POST $F HTTP/1.0\r\n"
                "Host: $F\r\n"
                "$F$S"
                "Accept: */*\r\n"
                "Content-Length: $D\r\n"
                "Content-Type: application/x-www-form-urlencoded\r\n"
                "\r\n"
                "$S"), client_urlbuf,
                client_hoststr,
                ahl != 0 ? ahl : PSTR(""),
                ahl != 0 ? "\r\n" : "",
                strlen(client_postval),
                client_postval);
        }
    }
    return bfill.position();
}

void EtherCardW5100::browseUrlRamHost(const char *urlbuf, const char *urlbuf_varpart, const char *hoststr, void(*callback)(uint8_t, uint16_t, uint16_t)) {
    browseUrlRamHost(urlbuf, urlbuf_varpart, hoststr, PSTR("Accept: text/html"), callback);
}

/// <summary>
/// tcpip.cpp - prepare and send HTTP request
/// </summary>
/// <param name="urlbuf">Pointer to c-string URL folder</param>
/// <param name="urlbuf_varpart">Pointer to c-string URL file</param>
/// <param name="hoststr">Pointer to c-string hostname</param>
/// <param name="callback">Pointer to callback function to handle response</param>
void EtherCardW5100::browseUrl(const char *urlbuf, const char *urlbuf_varpart, const char *hoststr, void(*callback) (uint8_t, uint16_t, uint16_t))
{
    browseUrl(urlbuf, urlbuf_varpart, hoststr, PSTR("Accept: text/html"), callback);
}

/// <summary>
/// tcpip.cpp - prepare and send HTTP request / the reply is received in the main packetloop
/// </summary>
/// <param name="urlbuf">Pointer to c-string URL folder</param>
/// <param name="urlbuf_varpart">Pointer to c-string URL file</param>
/// <param name="hoststr">Pointer to c-string hostname</param>
/// <param name="additionalheaderline">additionalheaderline Pointer to c-string with additional HTTP header info</param>
/// <param name="callback">callback Pointer to callback function to handle response</param>
void EtherCardW5100::browseUrl(const char *urlbuf, const char *urlbuf_varpart, const char *hoststr, const char *additionalheaderline, void(*callback) (uint8_t, uint16_t, uint16_t))
{
    client_urlbuf = urlbuf;
    client_urlbuf_var = urlbuf_varpart;
    client_hoststr = hoststr;
    client_additionalheaderline = additionalheaderline;
    client_postval = 0;
    client_browser_cb = callback;
    //    client_tcp_datafill_cb = &www_client_internal_datafill_cb;

        // check cb pointer is 'real' (non zero)
    if (!client_browser_cb)
        return;

    // fill the buffer
    uint16_t len = (*www_client_internal_datafill_cb) (www_fd);
    buffer[bfill.position() + TCP_OFFSET] = '\0';

    DEBUG_PRINT(F("Browse URL: "));

    for (uint16_t c = TCP_OFFSET; c < bfill.position() + TCP_OFFSET; c++)
    {
        // print readable ascii characters
        if (buffer[c] >= 0x08 && buffer[c] <= 0x0D)
        {
            DEBUG_PRINT((char)buffer[c]); // DEBUG_PRINT ( F ( " " ) );		// substitute a space so less rows in serial output
        }
        else if (buffer[c] > 0x1F)
        {
            DEBUG_PRINT((char)buffer[c]);
        }
    }
    DEBUG_PRINTLN(F(""));

    // close any connection before send a new request, to free the socket
    outgoing_client.stop();

    // send the request
    if (outgoing_client.connect(hoststr, hisport))
    {
        // send the HTTP GET request:
        outgoing_client.print((char*)bfill.buffer());
        outgoing_client.println();
        DEBUG_PRINT(F("Browse URL: sent to "));
        DEBUG_PRINT(hoststr);
        DEBUG_PRINT(F(" port "));
        DEBUG_PRINT(hisport);
        DEBUG_PRINTLN(F("(OK)"));
        outgoing_client_state = TCP_ESTABLISHED;
    }
    else
    {
        DEBUG_PRINTLN(F("Browse URL: failed (could not connect)"));
        outgoing_client_state = TCP_CLOSED;
    }
}

/// <summary>
/// tcp.cpp - send ping
/// </summary>
/// <param name="destip">Pointer to 4 byte destination IP address</param>
void EtherCardW5100::clientIcmpRequest(const uint8_t *destip)
{
    IPAddress pingAddr(destip[0], destip[1], destip[2], destip[3]); // ip address to ping

    DEBUG_PRINT(F("Ping: to "));
    DEBUG_PRINT(destip[0]);
    DEBUG_PRINT(F("."));
    DEBUG_PRINT(destip[1]);
    DEBUG_PRINT(F("."));
    DEBUG_PRINT(destip[2]);
    DEBUG_PRINT(F("."));
    DEBUG_PRINT(destip[3]);

    // note - asynchStart will return false if we couldn't even send a ping
    if (!ping.asyncStart(pingAddr, 3, ping_result))
    {
        DEBUG_PRINT(F(" send failed (status="));
        DEBUG_PRINT((int)ping_result.status);
        DEBUG_PRINTLN(F(")"));
    }
    else
    {
        DEBUG_PRINTLN(F(" sent (OK)"));
    }
}

/// <summary>
/// tcp.cpp - check for ping response
/// </summary>
/// <param name="ip_monitoredhost">Pointer to 4 byte IP address of host to check</param>
/// <returns>True (1) if ping response from specified host</returns>
uint8_t EtherCardW5100::packetLoopIcmpCheckReply(const uint8_t *ip_monitoredhost)
{
    if (ping.asyncComplete(ping_result))
    {
        DEBUG_PRINT(F("Ping: "));

        if (ping_result.status != SUCCESS)
        {
            // failure... but whyyyy?
            DEBUG_PRINT(F(" failed (status="));
            DEBUG_PRINT(ping_result.status);
            DEBUG_PRINTLN(F(")"));
            return 0;
        }
        else
        {
            // huzzah
            DEBUG_PRINT(F(" reply "));
            DEBUG_PRINT(ping_result.data.seq);
            DEBUG_PRINT(F(" from "));
            DEBUG_PRINT(ping_result.addr[0]);
            DEBUG_PRINT(F("."));
            DEBUG_PRINT(ping_result.addr[1]);
            DEBUG_PRINT(F("."));
            DEBUG_PRINT(ping_result.addr[2]);
            DEBUG_PRINT(F("."));
            DEBUG_PRINT(ping_result.addr[3]);
            DEBUG_PRINT(F(" bytes="));
            DEBUG_PRINT(REQ_DATASIZE);
            DEBUG_PRINT(F(" time="));
            DEBUG_PRINT(millis() - ping_result.data.time);
            DEBUG_PRINT(F(" TTL="));
            DEBUG_PRINT(ping_result.ttl);

            // check the address
            if (ping_result.addr[0] == ip_monitoredhost[0] &&
                ping_result.addr[1] == ip_monitoredhost[1] &&
                ping_result.addr[2] == ip_monitoredhost[2] &&
                ping_result.addr[3] == ip_monitoredhost[3])
            {
                DEBUG_PRINTLN(F(" (OK)"));
                return 1;
            }
            else
            {
                DEBUG_PRINTLN(F(" (received from wrong host)"));
                return 0;
            }
        }
    }
    return 0;
}

//=================================================================================
// dns.cpp
//=================================================================================

/// /// <summary>
/// dns.cpp - perform DNS lookup. Result is stored in hisip member.
/// </summary>
/// <param name="name">Host name to lookup</param>
/// <param name="fromRam">NOT IMPLEMENTED (Look up cached name. Default = false)</param>
/// <returns>True on success. </returns>
bool EtherCardW5100::dnsLookup(const char* name, bool fromRam)
{
    IPAddress serverIP(0, 0, 0, 0);

    dns_client.begin(Ethernet.dnsServerIP());

    int result = dns_client.getHostByName(name, serverIP);

    DEBUG_PRINT(F("DNS lookup "));
    DEBUG_PRINT(name);
    DEBUG_PRINT(F(" is "));
    DEBUG_PRINT(serverIP[0]);
    DEBUG_PRINT(F("."));
    DEBUG_PRINT(serverIP[1]);
    DEBUG_PRINT(F("."));
    DEBUG_PRINT(serverIP[2]);
    DEBUG_PRINT(F("."));
    DEBUG_PRINT(serverIP[3]);

    for (uint8_t i = 0; i < 4; i++)
        hisip[i] = serverIP[i];

    if (result == 1)
    {
        DEBUG_PRINTLN(F(" (OK)"));
        return true;
    }
    else
    {
        DEBUG_PRINTLN(F(" (failed)"));
        return false;
    }
}









