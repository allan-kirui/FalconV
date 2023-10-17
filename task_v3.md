# Task description
The 'https://open-meteo.com/' website provides an open API, that can be used for searching and collecting a detailed weather forecast for a selected location.
There are several measurements to check, including temperature, rain and snowfall, all encoded in the JSON format. The task is to design and code a service that queries for a given location,
collects measurements and logs an alert when thresholds for certain factors are reached.

See `https://open-meteo.com/en/docs` for more insight.

## Requirements
The service must run two separate threads, the producer (alias `WeatherFetcher`) and the consumer (alias `WeatherProcessor`).
The `WeatherFetcher` must query the open-meteo API and be able to pass the collected data to the `WeatherProcessor`.
The `WeatherProcessor` must analyze the received data and print alerts to stdout when:
 - the temperature is below 10 degrees of Celsius
 - the expected rainfall is greater than 0 mm

```
┌─────────────────────────────────┐
│        open-meteo API           │
└───────────────┬─────────────────┘
             ▲  │
             │  │ 2.open-meteo data (json)
  1.API call │  │
             │  │
             │  │
┌────────────┼──┼──────────────────────────────────────────────────────────────────┐
│            │  │                                                                  │
│            │  ▼                                                                  │
│    ┌───────┴────────────┐                           ┌────────────────────┐       │
│    │                    │                           │                    │       │
│    │                    │                           │                    │       │
│    │                    │     3.parsed data         │                    │   4.print      ┌───────────┐
│    │   WeatherFetcher   │  ─────────────────────►   │  WeatherProcessor  │  ─────┬─────►  │   stdout  │
│    │                    │                           │                    │       │        └───────────┘
│    │                    │                           │                    │       │
│    │                    │                           │                    │       │
│    └────────────────────┘                           └────────────────────┘       │
│          thread #1                                        thread #2              │
│                                                                                  │
│                                                                                  │
└──────────────────────────────────────────────────────────────────────────────────┘
 Service

```

### Open-meteo API details
* The example URL for Wrocław looks like that: `https://api.open-meteo.com/v1/forecast?latitude=51.10&longitude=17.03&hourly=temperature_2m,rain`.
* It returns a JSON-encoded document with a bunch of hourly metricts (see `https://jsonformatter.org/json-parser/f7131f`). The indexes in `.hourly.time` column correspond to the indexes in `.hourly.temperature_2m` and `.hourly.rain` columns respectively, i.e `.hourly.time[0]` is the timestamp of the `.hourly.temperature_2m[0]` and `.hourly.rain[0]` measurements, `.hourly.time[1]` corresponds to `.hourly.temperature_2m[1]` and `.hourly.rain[1]`, and so on.

### WeatherFetcher
The `WeatherFetcher`'s role is to:
* Call an open-meteo API (1). 
* Parse the received data and transform them to the in-code representation (2).
* Pass the parsed weather data to the `WeatherProcessor`, running on a separate thread (3)

### WeatherProcessor
The `WeatherProcessor`'s role is to:
* Wait for a weather data to come. No interaction back to the `WeatherFetcher` is expected.
* Filter the received weather data and print (to stdout) an alert when the certain thresholds are reached (4).

### Expected outcome
The report should print the timestamp of the forecast along with the detailed values of the measurements.

I.e.
```
---------------------------------REPORT------------------------------------
Warning Wroclaw, low temperature 5.900000 of C and rain 0.100000 mm expected on 2023-03-12T13:00
Warning Wroclaw, low temperature 5.400000 of C and rain 0.100000 mm expected on 2023-03-12T14:00
Warning Wroclaw, low temperature 5.000000 of C and rain 0.100000 mm expected on 2023-03-12T15:00
Warning Wroclaw, low temperature 5.000000 of C and rain 0.400000 mm expected on 2023-03-12T16:00
Warning Wroclaw, low temperature 5.100000 of C and rain 0.400000 mm expected on 2023-03-12T17:00
Warning Wroclaw, low temperature 5.400000 of C and rain 0.400000 mm expected on 2023-03-12T18:00
```

## Constraints
The service must be written in the C++ (up to 20). Only standard library and the boost 1.81.0 utilities may be used.
