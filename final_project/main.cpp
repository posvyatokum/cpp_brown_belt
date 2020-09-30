#include "test_runner.h"
#include "geo.h"
#include "transport_system.h"
#include "request.h"

#include <iostream>
#include <set>
#include <sstream>
#include <fstream>

using namespace std;

void TestCreate () {
    TransportSystem ts;
}

void TestAddStop () {
    TransportSystem ts;
    ASSERT_EQUAL(ts.AddStop("stop1", 0, 0)->id, 0);
    ASSERT_EQUAL(ts.GetStop(0)->name, "stop1");
    ASSERT_EQUAL(ts.AddStop("stop2", 0, 0)->id, 1);
    ASSERT_EQUAL(ts.GetStop(0)->name, "stop1");
    ASSERT_EQUAL(ts.GetStop(1)->name, "stop2");
}

void TestAddStopTwice() {
    TransportSystem ts;
    ASSERT_EQUAL(ts.AddStop("stop1", 0, 0)->id, 0);
    ASSERT_EQUAL(ts.GetStop(0)->name, "stop1");
    ASSERT_EQUAL(ts.AddStop("stop2", 0, 0)->id, 1);
    ASSERT_EQUAL(ts.GetStop(0)->name, "stop1");
    ASSERT_EQUAL(ts.GetStop(1)->name, "stop2");
    ASSERT_EQUAL(ts.AddStop("stop1", 0, 0)->id, 0);
    ASSERT_EQUAL(ts.GetStop(0)->name, "stop1");
    ASSERT_EQUAL(ts.GetStop(1)->name, "stop2");
}

void TestStopLonLat() {
    TransportSystem ts;
    ASSERT_EQUAL(ts.AddStop("stop1", 1, 2)->id, 0);
    ASSERT_EQUAL(ts.GetStop("stop1")->lat, 1);
    ASSERT_EQUAL(ts.GetStop("stop1")->lon, 2);
}

void TestAddRoundBus() {
    TransportSystem ts;
    vector<string> route = {
        "stop1",
        "stop2",
        "stop3"
    };
    for (const auto& stop_name : route) {
        ts.AddStop(stop_name, 0, 0);
    }
    auto bus = ts.AddRoundBus("bus1", route);
    ASSERT_EQUAL(bus->id, 0);
    ASSERT_EQUAL(bus->stops.size(), route.size());
    for (size_t i = 0; i < route.size(); ++i) {
        ASSERT_EQUAL(bus->stops[i]->name, route[i]);
    }
    ASSERT_EQUAL(ts.GetBus(0)->name, "bus1");
}

void TestAddStraightBus() {
    TransportSystem ts;
    vector<string> route = {
       "stop1",
       "stop2",
       "stop3"
    };
    for (const auto& stop_name : route) {
        ts.AddStop(stop_name, 0, 0);
    }
    auto bus = ts.AddStraightBus("bus2", route);
    ASSERT_EQUAL(bus->id, 0);
    ASSERT_EQUAL(bus->stops.size(), route.size());
    for (size_t i = 0; i < route.size(); ++i) {
        ASSERT_EQUAL(bus->stops[i]->name, route[i]);
    }
    ASSERT_EQUAL(ts.GetBus(0)->name, "bus2");

}

void TestAddRouteBeforeStops() {
    TransportSystem ts;
    vector<string> route = {
       "stop1",
       "stop2",
       "stop3"
    };
    {
        auto bus = ts.AddRoundBus("bus1", route);
        ASSERT_EQUAL(bus->id, 0);
        ASSERT_EQUAL(bus->stops.size(), route.size());
        for (size_t i = 0; i < route.size(); ++i) {
            ASSERT_EQUAL(bus->stops[i]->name, route[i]);
        }
        ASSERT_EQUAL(ts.GetBus(0)->name, "bus1");
    }
    {
        auto bus = ts.AddStraightBus("bus2", route);
        ASSERT_EQUAL(bus->id, 1);
        ASSERT_EQUAL(bus->stops.size(), route.size());
        for (size_t i = 0; i < route.size(); ++i) {
            ASSERT_EQUAL(bus->stops[i]->name, route[i]);
        }
        ASSERT_EQUAL(ts.GetBus(1)->name, "bus2");
    }
}

void TestAddDummyStop() {
    TransportSystem ts;
    ts.AddDummyStop("stop1");
    ASSERT_EQUAL(ts.GetStop("stop1")->lat, 0.0);
    ASSERT_EQUAL(ts.GetStop("stop1")->lon, 0.0);
    unordered_map<string, double> distances = {{"Marushkino", 3900}};
    ts.AddStop("stop1", 1, 1, distances);
    ASSERT_EQUAL(ts.GetStop("stop1")->lat, 1.0);
    ASSERT_EQUAL(ts.GetStop("stop1")->lon, 1.0);
    ASSERT_EQUAL(ts.GetStop("stop1")->distances, distances);
}

void TestDistance() {
    double lat1 = 0.0, lon1 = 0.0, lat2 = 10.0, lon2 = 10.0;
    double ans = 1569013.0;
    ASSERT(CalculateGeoDistance(lat1, lon1, lat2, lon2) - ans < 0.000001);
}

void TestRouteDistance() {
    TransportSystem ts;
    ts.AddStop("Tolstopaltsevo", 55.611087, 37.20829);
    ts.AddStop("Marushkino", 55.595884, 37.209755);
    ts.AddStop("Rasskazovka", 55.632761, 37.333324);

    ts.AddStraightBus("750", {"Tolstopaltsevo", "Marushkino", "Rasskazovka"});

    auto bus = ts.GetBus("750");

    ASSERT_EQUAL(bus->StopsCount(), 5);
    ASSERT_EQUAL(bus->UniqueStopsCount(), 3);
    ASSERT(bus->RouteLength() - 20939.5 < 1e-9);
}

void TestStopToBuses() {
    TransportSystem ts;
    for (int i = 1; i < 6; ++i) {
        ts.AddStop("stop" + to_string(i), 0, 0);
    }
    ts.AddStraightBus("1", {"stop1", "stop2", "stop3"});
    ts.AddRoundBus("2", {"stop1", "stop4", "stop5"});
    ts.AddStraightBus("3", {"stop1", "stop3", "stop4"});

    {
        auto buses = ts.GetBusesOnStop("stop1");
        set<string> buses_set;
        for (auto& bus : buses.value()) {
            buses_set.insert(bus->name);
        }
        ASSERT_EQUAL(buses_set.size(), 3);
        ASSERT(buses_set.count("1"));
        ASSERT(buses_set.count("2"));
        ASSERT(buses_set.count("3"));
    }
    {
        auto buses = ts.GetBusesOnStop("stop2");
        set<string> buses_set;
        for (auto& bus : buses.value()) {
            buses_set.insert(bus->name);
        }
        ASSERT_EQUAL(buses_set.size(), 1);
        ASSERT(buses_set.count("1"));
    }
    {
        auto buses = ts.GetBusesOnStop("stop4");
        set<string> buses_set;
        for (auto& bus : buses.value()) {
            buses_set.insert(bus->name);
        }
        ASSERT_EQUAL(buses_set.size(), 2);
        ASSERT(buses_set.count("2"));
        ASSERT(buses_set.count("3"));
    }
}

void TestAddStopWithDistance() {
    TransportSystem ts;
    unordered_map<string, double> distances = {{"Marushkino", 3900}};

    ts.AddStop("Tolstopaltsevo", 55.611087, 37.20829, distances);
    ASSERT_EQUAL(ts.GetStop("Tolstopaltsevo")->distances, distances);
}

void TestRouteDistanceWithManulDistance() {
    TransportSystem ts;
    {
        unordered_map<string, double> distances = {{"Marushkino", 3900}};
        ts.AddStop("Tolstopaltsevo", 55.611087, 37.20829, distances);
    }
    {
        unordered_map<string, double> distances = {{"Rasskazovka", 9900}};
        ts.AddStop("Marushkino", 55.595884, 37.209755, distances);
    }
    {
        ts.AddStop("Rasskazovka", 55.632761, 37.333324);
    }
    ts.AddStraightBus("750", {"Tolstopaltsevo", "Marushkino", "Rasskazovka"});
    ASSERT_EQUAL(ts.GetBus("750")->RouteLength(), 27600);
    ASSERT(abs(ts.GetBus("750")->Curvature() - 1.318084) < 1e-6);

}

void TestWriteRequestParseAddStop() {
    {
        stringstream request_stream = stringstream(
                    "{"
                    "\"type\": \"Stop\","
                    "\"name\": \"Tolstopaltsevo\","
                    "\"longitude\": 37.20829,"
                    "\"latitude\": 55.611087,"
                    "\"road_distances\": {}"
                    "}"
                    );
        Json::Document request_json = Json::Load(request_stream);
        RequestHolder request = make_shared<AddStopRequest>();
        request->ParseFrom(request_json.GetRoot());
        ASSERT_EQUAL(request->type, Request::Type::ADD_STOP);
        auto add_stop_request = static_cast<AddStopRequest*>(request.get());
        ASSERT_EQUAL(add_stop_request->stop_name, "Tolstopaltsevo");
        ASSERT_EQUAL(add_stop_request->lat, 55.611087);
        ASSERT_EQUAL(add_stop_request->lon, 37.20829);
    }
    {
        stringstream request_stream = stringstream(
                    "{"
                    "\"type\": \"Stop\","
                    "\"name\": \"Tolstopaltsevo\","
                    "\"longitude\": 37.20829,"
                    "\"latitude\": 55.611087,"
                    "\"road_distances\": {\"Marushkino\": 3900}"
                    "}"
                    );
        Json::Document request_json = Json::Load(request_stream);
        RequestHolder request = make_shared<AddStopRequest>();
        request->ParseFrom(request_json.GetRoot());
        ASSERT_EQUAL(request->type, Request::Type::ADD_STOP);
        auto add_stop_request = static_cast<AddStopRequest*>(request.get());
        unordered_map<string, double> distances = {{"Marushkino", 3900}};
        ASSERT_EQUAL(add_stop_request->distances, distances);
    }
}

void TestWriteRequestParseAddRoundBus() {
    {
        stringstream request_stream = stringstream(
                "{"
                "\"type\": \"Bus\","
                "\"name\": \"256\","
                "\"stops\": ["
                "\"Biryulyovo Zapadnoye\","
                "\"Biryusinka\","
                "\"Universam\","
                "\"Biryulyovo Tovarnaya\","
                "\"Biryulyovo Passazhirskaya\","
                "\"Biryulyovo Zapadnoye\""
                "],"
                "\"is_roundtrip\": true"
                "}"
                );

        Json::Document request_json = Json::Load(request_stream);
        RequestHolder request = make_shared<AddRoundBusRequest>();
        request->ParseFrom(request_json.GetRoot());
        ASSERT_EQUAL(request->type, Request::Type::ADD_ROUND_BUS);
        auto add_bus_request = static_cast<AddRoundBusRequest*>(request.get());
        ASSERT_EQUAL(add_bus_request->bus_name, "256");
        ASSERT_EQUAL(add_bus_request->stops.size(), 6);
        ASSERT_EQUAL(add_bus_request->stops[5], "Biryulyovo Zapadnoye");
    }
}

void TestWriteRequestParseAddStraightBus() {
    {
        stringstream request_stream = stringstream(
            "{"
            "\"type\": \"Bus\","
            "\"name\": \"750\","
            "\"stops\": ["
            "\"Tolstopaltsevo\","
            "\"Marushkino\","
            "\"Rasskazovka\""
            "],"
            "\"is_roundtrip\": false"
            "}"
            );
        Json::Document request_json = Json::Load(request_stream);
        RequestHolder request = make_shared<AddStraightBusRequest>();
        request->ParseFrom(request_json.GetRoot());
        ASSERT_EQUAL(request->type, Request::Type::ADD_STRAIGHT_BUS);
        auto add_bus_request = static_cast<AddStraightBusRequest*>(request.get());
        ASSERT_EQUAL(add_bus_request->bus_name, "750");
        ASSERT_EQUAL(add_bus_request->stops.size(), 3);
        ASSERT_EQUAL(add_bus_request->stops[2], "Rasskazovka");
    }
}

void TestParseWriteRequest() {
    {
        stringstream request_stream = stringstream(
                    "{"
                    "\"bus_wait_time\": 6,"
                    "\"bus_velocity\": 40"
                    "}"
                    );
        Json::Document request_json = Json::Load(request_stream);
        RequestHolder request;
        request = ParseWriteRequest(request_json.GetRoot());
        ASSERT_EQUAL(request->type, Request::Type::ADD_PARAMS);
        auto add_params_request = static_cast<AddParamsRequest*>(request.get());
        ASSERT_EQUAL(add_params_request->wait_time, 6);
        ASSERT_EQUAL(add_params_request->velocity, 40);
    }
    {
        stringstream request_stream = stringstream(
                    "{"
                    "\"type\": \"Stop\","
                    "\"name\": \"Tolstopaltsevo\","
                    "\"longitude\": 37.20829,"
                    "\"latitude\": 55.611087,"
                    "\"road_distances\": {}"
                    "}"
                    );
        Json::Document request_json = Json::Load(request_stream);
        RequestHolder request;
        request = ParseWriteRequest(request_json.GetRoot());
        ASSERT_EQUAL(request->type, Request::Type::ADD_STOP);
        auto add_stop_request = static_cast<AddStopRequest*>(request.get());
        ASSERT_EQUAL(add_stop_request->stop_name, "Tolstopaltsevo");
        ASSERT_EQUAL(add_stop_request->lat, 55.611087);
        ASSERT_EQUAL(add_stop_request->lon, 37.20829);
    }
    {
        stringstream request_stream = stringstream(
                "{"
                "\"type\": \"Bus\","
                "\"name\": \"256\","
                "\"stops\": ["
                "\"Biryulyovo Zapadnoye\","
                "\"Biryusinka\","
                "\"Universam\","
                "\"Biryulyovo Tovarnaya\","
                "\"Biryulyovo Passazhirskaya\","
                "\"Biryulyovo Zapadnoye\""
                "],"
                "\"is_roundtrip\": true"
                "}"
                );

        Json::Document request_json = Json::Load(request_stream);
        RequestHolder request;
        request = ParseWriteRequest(request_json.GetRoot());
        ASSERT_EQUAL(request->type, Request::Type::ADD_ROUND_BUS);
        auto add_bus_request = static_cast<AddRoundBusRequest*>(request.get());
        ASSERT_EQUAL(add_bus_request->bus_name, "256");
        ASSERT_EQUAL(add_bus_request->stops.size(), 6);
        ASSERT_EQUAL(add_bus_request->stops[5], "Biryulyovo Zapadnoye");
    }
    {
        stringstream request_stream = stringstream(
            "{"
            "\"type\": \"Bus\","
            "\"name\": \"750\","
            "\"stops\": ["
            "\"Tolstopaltsevo\","
            "\"Marushkino\","
            "\"Rasskazovka\""
            "],"
            "\"is_roundtrip\": false"
            "}"
            );
        Json::Document request_json = Json::Load(request_stream);
        RequestHolder request;
        request = ParseWriteRequest(request_json.GetRoot());
        ASSERT_EQUAL(request->type, Request::Type::ADD_STRAIGHT_BUS);
        auto add_bus_request = static_cast<AddStraightBusRequest*>(request.get());
        ASSERT_EQUAL(add_bus_request->bus_name, "750");
        ASSERT_EQUAL(add_bus_request->stops.size(), 3);
        ASSERT_EQUAL(add_bus_request->stops[2], "Rasskazovka");
    }
}

void TestProcessWriteRequest() {
    {
        TransportSystem ts;
        {
            auto request = make_shared<AddStopRequest>();
            request->stop_name = "stop1";
            request->lat = 0;
            request->lon = 0;
            request->Process(ts);
            ASSERT_EQUAL(ts.GetStop(0)->name, "stop1");
        }
        {
            auto request = make_shared<AddStopRequest>();
            request->stop_name = "stop2";
            request->lat = 0;
            request->lon = 0;
            request->Process(ts);
            ASSERT_EQUAL(ts.GetStop(0)->name, "stop1");
            ASSERT_EQUAL(ts.GetStop(1)->name, "stop2");
        }
        {
            auto request = make_shared<AddStopRequest>();
            request->stop_name = "stop1";
            request->lat = 0;
            request->lon = 0;
            request->Process(ts);
            ASSERT_EQUAL(ts.GetStop(0)->name, "stop1");
            ASSERT_EQUAL(ts.GetStop(1)->name, "stop2");
        }
        {
            auto request = make_shared<AddStopRequest>();
            request->stop_name = "stop2";
            request->lat = 0;
            request->lon = 0;
            request->Process(ts);
            ASSERT_EQUAL(ts.GetStop(0)->name, "stop1");
            ASSERT_EQUAL(ts.GetStop(1)->name, "stop2");
        }
    }

    {
        TransportSystem ts;
        {
            auto request = make_shared<AddStopRequest>();
            request->stop_name = "stop1";
            request->lat = 1;
            request->lon = 2;
            request->Process(ts);
            ASSERT_EQUAL(ts.GetStop("stop1")->lat, 1);
            ASSERT_EQUAL(ts.GetStop("stop1")->lon, 2);
        }
    }
    {
        TransportSystem ts;
        {
            unordered_map<string, double> distances = {{"Marushkino", 3900}};
            auto request = make_shared<AddStopRequest>();
            request->stop_name = "stop1";
            request->lat = 1;
            request->lon = 2;
            request->distances = distances;
            request->Process(ts);
            ASSERT_EQUAL(ts.GetStop("stop1")->lat, 1);
            ASSERT_EQUAL(ts.GetStop("stop1")->lon, 2);
            ASSERT_EQUAL(ts.GetStop("stop1")->distances, distances);
        }
    }
    {
        TransportSystem ts;
        vector<string> route = {
            "stop1",
            "stop2",
            "stop3"
        };
        for (const auto& stop_name : route) {
            auto request = make_shared<AddStopRequest>();
            request->stop_name = stop_name;
            request->lat = 0;
            request->lon = 0;
            request->Process(ts);
        }
        {
            auto request = make_shared<AddRoundBusRequest>();
            request->bus_name = "bus1";
            request->stops = route;
            request->Process(ts);
            auto bus = ts.GetBus("bus1");
            ASSERT_EQUAL(bus->id, 0);
            ASSERT_EQUAL(bus->stops.size(), route.size());
            for (size_t i = 0; i < route.size(); ++i) {
                ASSERT_EQUAL(bus->stops[i]->name, route[i]);
            }
            ASSERT_EQUAL(ts.GetBus(0)->name, "bus1");
        }
    }

    {
        TransportSystem ts;
        vector<string> route = {
            "stop1",
            "stop2",
            "stop3"
        };
        for (const auto& stop_name : route) {
            auto request = make_shared<AddStopRequest>();
            request->stop_name = stop_name;
            request->lat = 0;
            request->lon = 0;
            request->Process(ts);
        }
        {
            auto request = make_shared<AddStraightBusRequest>();
            request->bus_name = "bus1";
            request->stops = route;
            request->Process(ts);
            auto bus = ts.GetBus("bus1");
            ASSERT_EQUAL(bus->id, 0);
            ASSERT_EQUAL(bus->stops.size(), route.size());
            for (size_t i = 0; i < route.size(); ++i) {
                ASSERT_EQUAL(bus->stops[i]->name, route[i]);
            }
            ASSERT_EQUAL(ts.GetBus(0)->name, "bus1");
        }
    }
    {
        TransportSystem ts;
        auto request = make_shared<AddParamsRequest>();
        request->wait_time = 10.0;
        request->velocity = 60.0;
        request->Process(ts);
        ASSERT_EQUAL(ts.GetWaitTime(), 10.0);
        ASSERT_EQUAL(ts.GetVelocity(), 1000.0);
    }
}

void TestReadRequestParseBus() {
    stringstream request_stream = stringstream(
                "{"
                "\"type\": \"Bus\","
                "\"name\": \"256\","
                "\"id\": 519139350"
                "}"
                );
    Json::Document request_json = Json::Load(request_stream);
    RequestHolder request = make_shared<ReadBusRequest>();
    request->ParseFrom(request_json.GetRoot());
    ASSERT_EQUAL(request->type, Request::Type::READ_BUS);
    auto read_bus_request = static_cast<ReadBusRequest*>(request.get());
    ASSERT_EQUAL(read_bus_request->bus_name, "256");
}

void TestReadRequestParseStop() {
    stringstream request_stream = stringstream(
                "{"
                "\"type\": \"Stop\","
                "\"name\": \"Samara\","
                "\"id\": 746888088"
                "}"
                );
    Json::Document request_json = Json::Load(request_stream);
    RequestHolder request = make_shared<ReadStopRequest>();
    request->ParseFrom(request_json.GetRoot());
    ASSERT_EQUAL(request->type, Request::Type::READ_STOP);
    auto read_stop_request = static_cast<ReadStopRequest*>(request.get());
    ASSERT_EQUAL(read_stop_request->stop_name, "Samara");
}

void TestParseReadRequest() {
    {
        stringstream request_stream = stringstream(
                    "{"
                    "\"type\": \"Bus\","
                    "\"name\": \"256\","
                    "\"id\": 519139350"
                    "}"
                    );
        Json::Document request_json = Json::Load(request_stream);
        RequestHolder request;
        request = ParseReadRequest(request_json.GetRoot());
        ASSERT_EQUAL(request->type, Request::Type::READ_BUS);
        auto read_bus_request = static_cast<ReadBusRequest*>(request.get());
        ASSERT_EQUAL(read_bus_request->bus_name, "256");
        ASSERT_EQUAL(read_bus_request->request_id, 519139350);
    }
    {
        stringstream request_stream = stringstream(
                    "{"
                    "\"type\": \"Stop\","
                    "\"name\": \"Samara\","
                    "\"id\": 746888088"
                    "}"
                    );
        Json::Document request_json = Json::Load(request_stream);
        RequestHolder request;
        request = ParseReadRequest(request_json.GetRoot());
        ASSERT_EQUAL(request->type, Request::Type::READ_STOP);
        auto read_stop_request = static_cast<ReadStopRequest*>(request.get());
        ASSERT_EQUAL(read_stop_request->stop_name, "Samara");
        ASSERT_EQUAL(read_stop_request->request_id, 746888088);
    }
    {
        stringstream request_stream = stringstream(
                    "{"
                    "\"type\": \"Route\","
                    "\"from\": \"Biryulyovo Zapadnoye\","
                    "\"to\": \"Prazhskaya\","
                    "\"id\": 5"
                    "}"
                    );
        Json::Document request_json = Json::Load(request_stream);
        RequestHolder request;
        request = ParseReadRequest(request_json.GetRoot());
        ASSERT_EQUAL(request->type, Request::Type::READ_ROUTE);
        auto read_route_request = static_cast<ReadRouteRequest*>(request.get());
        ASSERT_EQUAL(read_route_request->from, "Biryulyovo Zapadnoye");
        ASSERT_EQUAL(read_route_request->to, "Prazhskaya");
    }
}

void TestProcessReadRequest() {
    TransportSystem ts;
    ts.AddStop("Tolstopaltsevo", 55.611087, 37.20829, {{"Marushkino", 3900}});
    ts.AddStop("Marushkino", 55.595884, 37.209755, {{"Rasskazovka", 9900}});
    ts.AddStop("Rasskazovka", 55.632761, 37.333324);
    ts.AddStop("Prazhskaya", 55.611678, 37.603831);

    ts.AddStraightBus("750", {"Tolstopaltsevo", "Marushkino", "Rasskazovka"});

    {
        stringstream request_stream = stringstream(
                    "{"
                    "\"type\": \"Bus\","
                    "\"name\": \"750\","
                    "\"id\": 519139350"
                    "}"
                    );
        Json::Document request_json = Json::Load(request_stream);
        RequestHolder request_holder;
        request_holder = ParseReadRequest(request_json.GetRoot());
        const auto& request = static_cast<const ReadBusRequest&>(*request_holder);
        Json::Node response = request.Process(ts);
        ASSERT_EQUAL(response.AsMap().size(), 5);
        ASSERT_EQUAL(response.AsMap().at("route_length").AsDouble(), 27600);
        ASSERT_EQUAL(response.AsMap().at("request_id").AsInt(), 519139350);
        ASSERT(abs(response.AsMap().at("curvature").AsDouble() - 1.318084) < 1e-5);
        ASSERT_EQUAL(response.AsMap().at("stop_count").AsInt(), 5);
        ASSERT_EQUAL(response.AsMap().at("unique_stop_count").AsInt(), 3);
    }
    {
        stringstream request_stream = stringstream(
                    "{"
                    "\"type\": \"Bus\","
                    "\"name\": \"751\","
                    "\"id\": 519139350"
                    "}"
                    );
        Json::Document request_json = Json::Load(request_stream);
        RequestHolder request_holder;
        request_holder = ParseReadRequest(request_json.GetRoot());
        const auto& request = static_cast<const ReadBusRequest&>(*request_holder);
        Json::Node response = request.Process(ts);
        ASSERT_EQUAL(response.AsMap().size(), 2);
        ASSERT_EQUAL(response.AsMap().at("error_message").AsString(), "not found");
        ASSERT_EQUAL(response.AsMap().at("request_id").AsInt(), 519139350);
    }
    {
        stringstream request_stream = stringstream(
                    "{"
                    "\"type\": \"Stop\","
                    "\"name\": \"Samara\","
                    "\"id\": 746888088"
                    "}"
                    );
        Json::Document request_json = Json::Load(request_stream);
        RequestHolder request_holder;
        request_holder = ParseReadRequest(request_json.GetRoot());
        const auto& request = static_cast<const ReadStopRequest&>(*request_holder);
        Json::Node response = request.Process(ts);
        ASSERT_EQUAL(response.AsMap().size(), 2);
        ASSERT_EQUAL(response.AsMap().at("error_message").AsString(), "not found");
        ASSERT_EQUAL(response.AsMap().at("request_id").AsInt(), 746888088);
    }
    {
        stringstream request_stream = stringstream(
                    "{"
                    "\"type\": \"Stop\","
                    "\"name\": \"Prazhskaya\","
                    "\"id\": 746888088"
                    "}"
                    );
        Json::Document request_json = Json::Load(request_stream);
        RequestHolder request_holder;
        request_holder = ParseReadRequest(request_json.GetRoot());
        const auto& request = static_cast<const ReadStopRequest&>(*request_holder);
        Json::Node response = request.Process(ts);
        ASSERT_EQUAL(response.AsMap().size(), 2);
        ASSERT_EQUAL(response.AsMap().at("buses").AsVector().size(), 0);
        ASSERT_EQUAL(response.AsMap().at("request_id").AsInt(), 746888088);
    }
    {
        stringstream request_stream = stringstream(
                    "{"
                    "\"type\": \"Stop\","
                    "\"name\": \"Tolstopaltsevo\","
                    "\"id\": 746888088"
                    "}"
                    );
        Json::Document request_json = Json::Load(request_stream);
        RequestHolder request_holder;
        request_holder = ParseReadRequest(request_json.GetRoot());
        const auto& request = static_cast<const ReadStopRequest&>(*request_holder);
        Json::Node response = request.Process(ts);
        ASSERT_EQUAL(response.AsMap().size(), 2);
        ASSERT_EQUAL(response.AsMap().at("buses").AsVector().size(), 1);
        ASSERT_EQUAL(response.AsMap().at("buses").AsVector().at(0).AsString(), "750");
        ASSERT_EQUAL(response.AsMap().at("request_id").AsInt(), 746888088);
    }
}

void TestFullFlow() {
    TransportSystem ts;
    stringstream out_stream;
    ifstream request_stream;
    request_stream.open("./full_flow_test.txt", ifstream::in);
    const auto [write_requests, read_requests] = ReadRequests(request_stream);
    ProcessWriteRequests(write_requests, ts);
    ts.BuildGraph();
    const auto responses = ProcessReadRequests(read_requests, ts);
    PrintResponses(responses, out_stream);

    ifstream ans_stream;
    ans_stream.open("./full_flow_ans.txt", ifstream::in);
    Json::Document model_ans = Json::Load(ans_stream);
    Json::Document our_ans = Json::Load(out_stream);
    ASSERT_EQUAL(our_ans, model_ans);
}

void TestLoadJson() {
    stringstream stream;
    stream
        << "{"
        << "\"type\": \"Bus\","
        << "\"name\": \"256\","
        << "\"stops\": ["
        <<    "\"Biryulyovo Zapadnoye\","
        <<    "\"Biryusinka\""
        << "],"
        << "\"is_roundtrip\": true,"
        << "\"road_distances\": {"
        <<    "\"Rasskazovka\": 9900"
        << "},"
        << "\"longitude\": 37.209755"
        << "}";
    Json::Document document = Json::Load(stream);
    ASSERT_EQUAL(document.GetRoot().AsMap().size(), 6);
    ASSERT_EQUAL(document.GetRoot().AsMap().at("type").AsString(), "Bus");
    ASSERT_EQUAL(document.GetRoot().AsMap().at("name").AsString(), "256");
    ASSERT_EQUAL(document.GetRoot().AsMap().at("stops").AsVector().size(), 2);

    ASSERT_EQUAL(document.GetRoot().AsMap().at("is_roundtrip").AsBool(), true);

    ASSERT_EQUAL(document.GetRoot().AsMap().at("road_distances").AsMap().size(), 1);
    ASSERT_EQUAL(document.GetRoot().AsMap().at("road_distances").AsMap().at("Rasskazovka").AsInt(), 9900);
    ASSERT_EQUAL(document.GetRoot().AsMap().at("longitude").AsDouble(), 37.209755);
}

int main() {
    cout.precision(6);

    /*
    TestRunner tr;
    RUN_TEST(tr, TestCreate);
    RUN_TEST(tr, TestAddStop);
    RUN_TEST(tr, TestAddStopTwice);
    RUN_TEST(tr, TestAddDummyStop);
    RUN_TEST(tr, TestStopLonLat);
    RUN_TEST(tr, TestAddRoundBus);
    RUN_TEST(tr, TestAddStraightBus);
    RUN_TEST(tr, TestAddRouteBeforeStops);
    RUN_TEST(tr, TestDistance);
    RUN_TEST(tr, TestRouteDistance);
    RUN_TEST(tr, TestStopToBuses);
    RUN_TEST(tr, TestAddStopWithDistance);
    RUN_TEST(tr, TestRouteDistanceWithManulDistance);

    RUN_TEST(tr, TestLoadJson);

    RUN_TEST(tr, TestWriteRequestParseAddStop);
    RUN_TEST(tr, TestWriteRequestParseAddRoundBus);
    RUN_TEST(tr, TestWriteRequestParseAddStraightBus);
    RUN_TEST(tr, TestParseWriteRequest);
    RUN_TEST(tr, TestProcessWriteRequest);

    RUN_TEST(tr, TestReadRequestParseBus);
    RUN_TEST(tr, TestReadRequestParseStop);
    RUN_TEST(tr, TestParseReadRequest);
    RUN_TEST(tr, TestProcessReadRequest);
    */

    // RUN_TEST(tr, TestFullFlow);

    TransportSystem ts;
    const auto [write_requests, read_requests] = ReadRequests();
    ProcessWriteRequests(write_requests, ts);
    ts.BuildGraph();
    const auto responses = ProcessReadRequests(read_requests, ts);
    PrintResponses(responses);

    return 0;
}
