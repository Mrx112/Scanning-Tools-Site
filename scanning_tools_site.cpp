// scanning_tools.cpp
#include <iostream>
#include <string>
#include <vector>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <sstream>
#include <curl/curl.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/select.h>

// Helper for CURL response
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Resolve domain to IP
std::string resolve_domain(const std::string& domain) {
    struct addrinfo hints{}, *res;
    hints.ai_family = AF_INET;
    if (getaddrinfo(domain.c_str(), nullptr, &hints, &res) != 0) return "";
    char ipstr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &((struct sockaddr_in*)res->ai_addr)->sin_addr, ipstr, sizeof(ipstr));
    freeaddrinfo(res);
    return std::string(ipstr);
}

// Extract value from JSON string
std::string extract_json_value(const std::string& json, const std::string& key) {
    size_t pos = json.find("\"" + key + "\":");
    if (pos == std::string::npos) return "";

    pos += key.length() + 3;

    // Find the start of the value
    while (pos < json.length() && (json[pos] == ' ' || json[pos] == '"')) {
        pos++;
    }

    size_t end_pos = pos;
    if (json[pos-1] == '"') {
        // Value is in quotes
        end_pos = json.find("\"", pos);
        if (end_pos == std::string::npos) return "";
        return json.substr(pos, end_pos - pos);
    } else {
        // Value is not in quotes (number)
        while (end_pos < json.length() &&
            json[end_pos] != ',' &&
            json[end_pos] != '}' &&
            json[end_pos] != ' ') {
            end_pos++;
            }
            return json.substr(pos, end_pos - pos);
    }
}

// Get server location using multiple services
bool get_ip_location(const std::string& ip, std::string& country, std::string& region,
                     std::string& city, std::string& isp, std::string& org,
                     std::string& lat, std::string& lon) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cout << "Error: CURL initialization failed" << std::endl;
        return false;
    }

    std::string readBuffer;
    std::vector<std::string> endpoints = {
        "http://ip-api.com/line/" + ip + "?fields=status,country,regionName,city,isp,org,lat,lon",
        "https://ipapi.co/" + ip + "/json/",
        "http://ipinfo.io/" + ip + "/json"
    };

    bool success = false;

    for (const auto& url : endpoints) {
        readBuffer.clear();
        std::string service_name;
        if (url.find("ip-api.com") != std::string::npos) service_name = "ip-api.com";
        else if (url.find("ipapi.co") != std::string::npos) service_name = "ipapi.co";
        else service_name = "ipinfo.io";

        std::cout << "Trying: " << service_name << std::endl;

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36");
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

        if (url.find("https://") != std::string::npos) {
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        }

        CURLcode res = curl_easy_perform(curl);

        if (res == CURLE_OK && !readBuffer.empty()) {
            // Debug: show first 200 characters of response
            // std::cout << "Response: " << readBuffer.substr(0, 200) << "..." << std::endl;

            // Parse ip-api.com line format
            if (service_name == "ip-api.com") {
                std::istringstream response_stream(readBuffer);
                std::string line;
                int line_num = 0;

                while (std::getline(response_stream, line)) {
                    switch (line_num) {
                        case 0: success = (line == "success"); break;
                        case 1: country = line; break;
                        case 2: region = line; break;
                        case 3: city = line; break;
                        case 4: isp = line; break;
                        case 5: org = line; break;
                        case 6: lat = line; break;
                        case 7: lon = line; break;
                    }
                    line_num++;
                }
                if (success && line_num >= 8) break;
            }
            // Parse ipapi.co JSON format
            else if (service_name == "ipapi.co") {
                country = extract_json_value(readBuffer, "country_name");
                region = extract_json_value(readBuffer, "region");
                city = extract_json_value(readBuffer, "city");
                isp = extract_json_value(readBuffer, "org");
                org = isp;
                lat = extract_json_value(readBuffer, "latitude");
                lon = extract_json_value(readBuffer, "longitude");

                success = !country.empty();
                if (success) break;
            }
            // Parse ipinfo.io JSON format
            else if (service_name == "ipinfo.io") {
                country = extract_json_value(readBuffer, "country");
                region = extract_json_value(readBuffer, "region");
                city = extract_json_value(readBuffer, "city");
                org = extract_json_value(readBuffer, "org");
                isp = org;

                // Extract latitude and longitude from "loc" field
                std::string loc = extract_json_value(readBuffer, "loc");
                if (!loc.empty()) {
                    size_t comma_pos = loc.find(',');
                    if (comma_pos != std::string::npos) {
                        lat = loc.substr(0, comma_pos);
                        lon = loc.substr(comma_pos + 1);
                    }
                }

                success = !country.empty();
                if (success) break;
            }
        }

        sleep(1);
    }

    curl_easy_cleanup(curl);
    return success;
                     }

                     bool is_port_open(const std::string& ip, int port, int timeout_sec = 1) {
                         int sock = socket(AF_INET, SOCK_STREAM, 0);
                         if (sock < 0) return false;

                         struct sockaddr_in addr;
                         addr.sin_family = AF_INET;
                         addr.sin_port = htons(port);
                         inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

                         // Set socket to non-blocking
                         int flags = fcntl(sock, F_GETFL, 0);
                         fcntl(sock, F_SETFL, flags | O_NONBLOCK);

                         int result = connect(sock, (struct sockaddr*)&addr, sizeof(addr));

                         if (result == 0) {
                             close(sock);
                             return true;
                         }

                         if (errno == EINPROGRESS) {
                             fd_set writefds;
                             FD_ZERO(&writefds);
                             FD_SET(sock, &writefds);

                             struct timeval tv;
                             tv.tv_sec = timeout_sec;
                             tv.tv_usec = 0;

                             result = select(sock + 1, NULL, &writefds, NULL, &tv);

                             if (result > 0) {
                                 int error = 0;
                                 socklen_t len = sizeof(error);
                                 getsockopt(sock, SOL_SOCKET, SO_ERROR, &error, &len);
                                 close(sock);
                                 return error == 0;
                             }
                         }

                         close(sock);
                         return false;
                     }

                     int main() {
                         // Display banner
                         std::cout << "==============================================" << std::endl;
                         std::cout << "           SCANNING TOOLS v1.0" << std::endl;
                         std::cout << "           Created By Mrx_112" << std::endl;
                         std::cout << "==============================================" << std::endl;
                         std::cout << std::endl;

                         curl_global_init(CURL_GLOBAL_DEFAULT);

                         std::string website;
                         std::cout << "Masukkan nama website (contoh: Example.com): ";
                         std::getline(std::cin, website);
                         std::cout << "Resolving domain: " << website << std::endl;

                         std::string ip = resolve_domain(website);
                         if (ip.empty()) {
                             std::cout << "Failed to resolve domain." << std::endl;
                             curl_global_cleanup();
                             return 1;
                         }
                         std::cout << "IP Address: " << ip << std::endl;

                         std::cout << "Getting server location..." << std::endl;

                         std::string country, region, city, isp, org, lat, lon;
                         bool location_success = get_ip_location(ip, country, region, city, isp, org, lat, lon);

                         if (location_success) {
                             std::cout << "\nServer Location/Info:" << std::endl;
                             std::cout << "  Country   : " << (country.empty() ? "N/A" : country) << std::endl;
                             std::cout << "  Region    : " << (region.empty() ? "N/A" : region) << std::endl;
                             std::cout << "  City      : " << (city.empty() ? "N/A" : city) << std::endl;
                             std::cout << "  ISP       : " << (isp.empty() ? "N/A" : isp) << std::endl;
                             std::cout << "  Org       : " << (org.empty() ? "N/A" : org) << std::endl;
                             std::cout << "  Latitude  : " << (lat.empty() ? "N/A" : lat) << std::endl;
                             std::cout << "  Longitude : " << (lon.empty() ? "N/A" : lon) << std::endl;
                         } else {
                             std::cout << "\nWarning: Could not retrieve detailed location information." << std::endl;
                             std::cout << "This could be due to:" << std::endl;
                             std::cout << "1. Network connectivity issues" << std::endl;
                             std::cout << "2. GeoIP service limitations" << std::endl;
                             std::cout << "3. Cloudflare/CDN protection (IP: " << ip << ")" << std::endl;
                         }

                         std::vector<int> ports = {80, 443, 8080, 8443, 21, 22, 25, 110, 143, 3306, 3389};
                         std::cout << "\nScanning common ports..." << std::endl;

                         int open_ports = 0;
                         for (int port : ports) {
                             if (is_port_open(ip, port, 2)) { // Increased timeout to 2 seconds
                                 std::cout << "✓ Port " << port << " is OPEN";
                                 if (port == 80 || port == 8080) std::cout << " (HTTP)";
                                 else if (port == 443 || port == 8443) std::cout << " (HTTPS)";
                                 else if (port == 21) std::cout << " (FTP)";
                                 else if (port == 22) std::cout << " (SSH)";
                                 else if (port == 25) std::cout << " (SMTP)";
                                 else if (port == 110) std::cout << " (POP3)";
                                 else if (port == 143) std::cout << " (IMAP)";
                                 else if (port == 3306) std::cout << " (MySQL)";
                                 else if (port == 3389) std::cout << " (RDP)";
                                 std::cout << std::endl;
                                 open_ports++;
                             }
                         }

                         if (open_ports == 0) {
                             std::cout << "No common ports are open." << std::endl;
                         } else {
                             std::cout << "\nFound " << open_ports << " open port(s)." << std::endl;
                         }

                         curl_global_cleanup();
                         return 0;
                     }
