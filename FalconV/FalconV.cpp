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
#include <boost/iterator/zip_iterator.hpp>
#include <boost/tuple/tuple.hpp>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace json = boost::json;
using tcp = net::ip::tcp;

const std::string API_URL = "api.open-meteo.com";
const std::string API_PATH = "/v1/forecast";
const std::string LOCATION = "latitude=51.10&longitude=17.03";
const std::string WEATHER_VARS = "&hourly=temperature_2m,rain";

// Shared resource for weather data
json::value weather_data;
// mutex for synchronization
std::mutex location_mutex; 
std::mutex weather_data_mutex;

// Function to process weather data and raise alerts
void process_weather_data(const std::string& latitude, const std::string& longitude) {
    while (true) {
        // Check if weather_data is valid
        if (!weather_data.is_null()) {
            auto temperature_2m_arr = weather_data.at("hourly").at("temperature_2m").as_array();
            auto rain_arr = weather_data.at("hourly").at("rain").as_array();
            auto date_and_time_arr = weather_data.at("hourly").at("time").as_array();

            auto begin = boost::make_zip_iterator(boost::make_tuple(temperature_2m_arr.begin(), rain_arr.begin(), date_and_time_arr.begin()));
            auto end = boost::make_zip_iterator(boost::make_tuple(temperature_2m_arr.end(), rain_arr.end(), date_and_time_arr.end()));

            std::cout << "----------------Weather data report----------------" << std::endl;
            std::cout << "----------------Location: " << latitude << ", " << longitude << "----------------" << std::endl;
            for (auto it = begin; it != end; ++it) {
                auto temperature = boost::get<0>(*it).as_double();
                auto rainfall = boost::get<1>(*it).as_double();
                auto date_and_time = boost::get<2>(*it).as_string().c_str();
                if (temperature < 10.0 || rainfall > 0.0) {
                    std::cout << "Warning low temperature: " << temperature << " Celsius, and rainfall: " << rainfall << " mm expected on " << date_and_time << std::endl;
                }
            }
            std::cout << std::endl;
        }

        // Sleep for a while before checking weather_data again
        std::this_thread::sleep_for(std::chrono::seconds(20)); // Adjust sleep duration as needed
    }
}

// Function to fetch weather data from the API
void fetch_weather_data(const std::string& latitude, const std::string& longitude) {
    while (true) {
        try {
            auto const host = API_URL;
            auto const port = "80";
            auto const target = API_PATH + "?latitude=" + latitude + "&longitude=" + longitude + WEATHER_VARS;
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
            http::request<http::string_body> req{http::verb::get, target, version};
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
            //std::cout << res.body() << std::endl;

            // Parse the response and update weather_data
            weather_data = json::parse(res.body());

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

        // Sleep before fetching data again
        std::this_thread::sleep_for(std::chrono::seconds(20)); // Adjust sleep duration as needed
    }
}

// Function to get user input for latitude and longitude
std::pair<std::string, std::string> get_user_location(const std::string& previous_latitude, const std::string& previous_longitude) {
    std::string latitude = previous_latitude;
    std::string longitude = previous_longitude;

    std::string input;

    {
        std::lock_guard<std::mutex> lock(location_mutex); // Lock the mutex

        std::cout << "Enter latitude (-90.0 to 90.0, Press Enter to keep previous value " << previous_latitude << "): ";
        std::getline(std::cin, input);
        if (!input.empty()) {
            double new_latitude = std::stod(input);
            if (new_latitude >= -90.0 && new_latitude <= 90.0) {
                latitude = input;
            }
            else {
                std::cout << "Invalid latitude value. Latitude must be between -90.0 and 90.0 degrees." << std::endl;
            }
        }

        input.clear(); // Clear previous input

        std::cout << "Enter longitude (-180.0 to 180.0, Press Enter to keep previous value " << previous_longitude << "): ";
        std::getline(std::cin, input);
        if (!input.empty()) {
            double new_longitude = std::stod(input);
            if (new_longitude >= -180.0 && new_longitude <= 180.0) {
                longitude = input;
            }
            else {
                std::cout << "Invalid longitude value. Longitude must be between -180.0 and 180.0 degrees." << std::endl;
            }
        }
    }

    return { latitude, longitude };
}


// Function to handle user input for changing location
void user_input_thread(std::string& latitude, std::string& longitude) {
    while (true) {
        std::cout << "Current location: " << latitude << ", " << longitude << std::endl;
        std::cout << "\nDo you want to fetch weather details for a different location? (y/n): ";
        std::string input;
        std::getline(std::cin, input);

        if (input == "y" || input == "Y") {
            std::string new_latitude, new_longitude;
            std::tie(new_latitude, new_longitude) = get_user_location(latitude, longitude);

            if (!new_latitude.empty() && !new_longitude.empty()) {
                std::lock_guard<std::mutex> lock(location_mutex); // Lock the mutex
                latitude = new_latitude;
                longitude = new_longitude;
            }
        }

        // Sleep for specified time before checking again
        std::this_thread::sleep_for(std::chrono::seconds(60));
    }
}

int main() {
    //Location: Wroclaw, Poland
    std::string latitude = "51.10"; 
    std::string longitude = "17.03";


    while (true) {
        std::thread fetcher_thread(fetch_weather_data, std::cref(latitude), std::cref(longitude));
        std::lock_guard<std::mutex> lock(weather_data_mutex);
        std::thread processor_thread(process_weather_data, std::cref(latitude), std::cref(longitude));

        // Start user input thread
        std::thread user_input(user_input_thread, std::ref(latitude), std::ref(longitude));

        // Wait for threads to finish
        fetcher_thread.join();
        processor_thread.join();
        user_input.join();
    }
    return 0;
}
