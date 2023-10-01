#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <string>
#include <boost/asio.hpp>
#include <boost/json.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace json = boost::json;
using tcp = net::ip::tcp;

const std::string API_URL = "api.open-meteo.com";
const std::string API_PATH = "/v1/forecast";
const std::string LOCATION = "latitude=51.10&longitude=17.03&hourly=temperature_2m,rain";


// Function to process weather data and raise alerts
void process_weather_data() {
}

// Function to fetch weather data from the API
int fetch_weather_data() {
    try {
        auto const host = API_URL;
        auto const port = "80";
        auto const target = API_PATH + "?" + LOCATION;
        int version = 11;

        // The io_context is required for all I/O
        net::io_context ioc;
        // These objects perform our I/O
        tcp::resolver resolver(ioc);
        beast::tcp_stream stream(ioc);

        // Look up the domain name
        auto const results = resolver.resolve(host, port);

        // Make the connection on the IP address we get from a lookup
        stream.connect(results);

        // Set up an HTTP GET request message
        http::request<http::string_body> req{http::verb::get, target,version};
        req.set(http::field::host, host);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

        // Send the HTTP request to the remote host
        http::write(stream, req);

        // This buffer is used for reading and must be persisted
        beast::flat_buffer buffer;

        // Declare a container to hold the response
        http::response<http::string_body> res;

        // Receive the HTTP response
        http::read(stream, buffer, res);

        // Write the message to standard out
        std::cout << res.body() << std::endl;
        auto j = json::parse(res.body());
        std::cout << j.at("hourly_units").at("rain") << std::endl;
        // Gracefully close the socket
        beast::error_code ec;
        stream.socket().shutdown(tcp::socket::shutdown_both, ec);

        // not_connected happens sometimes
        // so don't bother reporting it.
        //
        if (ec && ec != beast::errc::not_connected)
            throw beast::system_error{ec};


    }// If we get here then the connection is closed gracefully
    catch (std::exception& e) {
        std::cerr << "Error fetching weather data: " << e.what() << std::endl;
    }
}

int main() {
    while (true) {
        fetch_weather_data();
        std::this_thread::sleep_for(std::chrono::minutes(10)); // Fetch data every 10 minutes
    }
    return 0;
}
