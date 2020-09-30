#include "request.h"

#include <sstream>
#include <set>

using namespace std;

RequestHolder Request::Create(Request::Type type) {
  switch (type) {
    case Request::Type::ADD_STOP:
      return make_shared<AddStopRequest>();
    case Request::Type::ADD_ROUND_BUS:
      return make_shared<AddRoundBusRequest>();
    case Request::Type::ADD_STRAIGHT_BUS:
      return make_shared<AddStraightBusRequest>();
    case Request::Type::ADD_PARAMS:
      return make_shared<AddParamsRequest>();
    case Request::Type::READ_BUS:
      return make_shared<ReadBusRequest>();
    case Request::Type::READ_STOP:
      return make_shared<ReadStopRequest>();
    case Request::Type::READ_ROUTE:
      return make_shared<ReadRouteRequest>();
    default:
      return nullptr;
  }
}

void AddParamsRequest::ParseFrom(const Json::Node& node) {
    wait_time = node.AsMap().at("bus_wait_time").AsDouble();
    velocity = node.AsMap().at("bus_velocity").AsDouble();
}

void AddParamsRequest::Process(TransportSystem& ts) const {
    ts.SetParams(wait_time, velocity * 1000.0 / 60.0);
}

void AddStopRequest::ParseFrom(const Json::Node& node)  {
    stop_name = node.AsMap().at("name").AsString();
    lat = node.AsMap().at("latitude").AsDouble();
    lon = node.AsMap().at("longitude").AsDouble();
    for (const auto& [other_stop_name, distance_node] : node.AsMap().at("road_distances").AsMap()) {
        distances[other_stop_name] = distance_node.AsDouble();
    }
}

void AddStopRequest::Process(TransportSystem& ts) const  {
    ts.AddStop(stop_name, lat, lon, distances);
}

void AddRoundBusRequest::ParseFrom(const Json::Node& node)  {
    bus_name = node.AsMap().at("name").AsString();
    for (const auto& stop_node : node.AsMap().at("stops").AsVector()) {
        stops.push_back(stop_node.AsString());
    }
}

void AddRoundBusRequest::Process(TransportSystem& ts) const  {
    ts.AddRoundBus(bus_name, stops);
}

void AddStraightBusRequest::ParseFrom(const Json::Node& node)  {
    bus_name = node.AsMap().at("name").AsString();
    for (const auto& stop_node : node.AsMap().at("stops").AsVector()) {
        stops.push_back(stop_node.AsString());
    }
}

void AddStraightBusRequest::Process(TransportSystem& ts) const  {
    ts.AddStraightBus(bus_name, stops);
}

void ReadBusRequest::ParseFrom(const Json::Node& node) {
    request_id = node.AsMap().at("id").AsInt();
    bus_name = node.AsMap().at("name").AsString();
}

Json::Node ReadBusRequest::Process(const TransportSystem& ts) const {
    map<string, Json::Node> result;
    result["request_id"] = Json::Node(double(request_id));

    auto bus = ts.GetBus(bus_name);
    if (!bus) {
        result["error_message"] = Json::Node(string("not found"));
        return Json::Node(result);
    }
    result["stop_count"] = Json::Node(double(bus->StopsCount()));
    result["unique_stop_count"] = Json::Node(double(bus->UniqueStopsCount()));
    result["route_length"] = Json::Node(bus->RouteLength());
    result["curvature"] = Json::Node(bus->Curvature());

    return Json::Node(result);
}

void ReadStopRequest::ParseFrom(const Json::Node& node) {
    request_id = node.AsMap().at("id").AsInt();
    stop_name = node.AsMap().at("name").AsString();
}

Json::Node ReadStopRequest::Process(const TransportSystem& ts) const {
    map<string, Json::Node> result;
    result["request_id"] = Json::Node(double(request_id));

    if (!ts.GetStop(stop_name)) {
        result["error_message"] = Json::Node(string("not found"));
        return Json::Node(result);
    }
    auto buses = ts.GetBusesOnStop(stop_name);
    if (!buses) {
        vector<Json::Node> buses_node_vector;
        result["buses"] = Json::Node(buses_node_vector);
        return Json::Node(result);
    }
    set<string> bus_names;
    for (const auto& bus : buses.value()) {
        bus_names.insert(bus->name);
    }
    vector<Json::Node> buses_node_vector;
    for (const auto& bus : bus_names) {
        buses_node_vector.push_back(Json::Node(bus));
    }
    result["buses"] = Json::Node(buses_node_vector);
    return Json::Node(result);
}

void ReadRouteRequest::ParseFrom(const Json::Node& node) {
    request_id = node.AsMap().at("id").AsInt();
    from = node.AsMap().at("from").AsString();
    to = node.AsMap().at("to").AsString();
}

Json::Node ReadRouteRequest::Process(const TransportSystem& ts) const {
    map<string, Json::Node> result;
    result["request_id"] = Json::Node(double(request_id));

    size_t from_id = ts.GetStop(from)->id;
    size_t to_id = ts.GetStop(to)->id;
    auto route = ts.router->BuildRoute(from_id * 2, to_id * 2);

    if (!route) {
        result["error_message"] = Json::Node(string("not found"));
    } else {
        result["total_time"] = route->weight;
        vector<Json::Node> items;
        for (size_t i = 0; i < route->edge_count; ++i) {
            items.push_back(ts.GetEdgeDescription(ts.router->GetRouteEdge(route->id, i)));
        }
        result["items"] = Json::Node(items);
    }

    return Json::Node(result);
}

optional<Request::Type> ReadWriteRequestTypeFromString(string_view& request_str) {
    string_view type_str = ReadToken(request_str, " ");
    if (type_str == "Stop") {
        return Request::Type::ADD_STOP;
    } else if (type_str == "Bus") {
        const size_t pos = request_str.find(">");
        if (pos == request_str.npos) {
            return Request::Type::ADD_STRAIGHT_BUS;
        } else {
            return Request::Type::ADD_ROUND_BUS;
        }
    } else {
        return nullopt;
    }
}

optional<Request::Type> ReadWriteRequestTypeFromJson(const Json::Node& request_json) {
    if (!request_json.AsMap().count("type")) {
        return Request::Type::ADD_PARAMS;
    }
    string type_str = request_json.AsMap().at("type").AsString();
    if (type_str == "Stop") {
        return Request::Type::ADD_STOP;
    } else if (type_str == "Bus") {
        if (!request_json.AsMap().at("is_roundtrip").AsBool()) {
            return Request::Type::ADD_STRAIGHT_BUS;
        } else {
            return Request::Type::ADD_ROUND_BUS;
        }
    } else {
        return nullopt;
    }
}

RequestHolder ParseWriteRequest(const Json::Node& request_json) {
    const auto request_type = ReadWriteRequestTypeFromJson(request_json);
    if (!request_type) {
        return nullptr;
    }
    RequestHolder request = Request::Create(*request_type);
    if (request) {
        request->ParseFrom(request_json);
    };
    return request;
}

optional<Request::Type> ReadReadRequestTypeFromString(string_view& request_str) {
    string_view type_str = ReadToken(request_str, " ");
    if (type_str == "Bus") {
        return Request::Type::READ_BUS;
    } else if (type_str == "Stop") {
        return Request::Type::READ_STOP;
    } else if (type_str == "Route") {
        return Request::Type::READ_ROUTE;
    } else {
        return nullopt;
    }
}

optional<Request::Type> ReadReadRequestTypeFromJson(const Json::Node& request_json) {
    string_view type_str = request_json.AsMap().at("type").AsString();
    if (type_str == "Bus") {
        return Request::Type::READ_BUS;
    } else if (type_str == "Stop") {
        return Request::Type::READ_STOP;
    } else if (type_str == "Route") {
        return Request::Type::READ_ROUTE;
    } else {
        return nullopt;
    }
}


RequestHolder ParseReadRequest(const Json::Node& request_json) {
    const auto request_type = ReadReadRequestTypeFromJson(request_json);
    if (!request_type) {
        return nullptr;
    }
    RequestHolder request = Request::Create(*request_type);
    if (request) {
        request->ParseFrom(request_json);
    }
    return request;
}

pair<vector<RequestHolder>, vector<RequestHolder>> ReadRequests(istream& in_stream) {
    vector<RequestHolder> write_requests, read_requests;
    Json::Document requests_json = Json::Load(in_stream);
    {
        write_requests.push_back(ParseWriteRequest(requests_json.GetRoot().AsMap().at("routing_settings")));
    }
    for (const auto& request_json : requests_json.GetRoot().AsMap().at("base_requests").AsVector()) {
        write_requests.push_back(ParseWriteRequest(request_json));
    }
    for (const auto& request_json : requests_json.GetRoot().AsMap().at("stat_requests").AsVector()) {
        read_requests.push_back(ParseReadRequest(request_json));
    }
    return {write_requests, read_requests};
}
/*
vector<RequestHolder> ReadWriteRequests(istream& in_stream) {
    const size_t request_count = ReadNumberOnLine<size_t>(in_stream);

    vector<RequestHolder> requests;
    requests.reserve(request_count);

    for (size_t i = 0; i < request_count; ++i) {
        string request_str;
        getline(in_stream, request_str);
        if (auto request = ParseWriteRequest(request_str)) {
            requests.push_back(move(request));
        }
    }
    return requests;
}
*/

void ProcessWriteRequests(const vector<RequestHolder>& requests, TransportSystem& ts) {
    for (const auto& request_holder : requests) {
        const auto& request = static_cast<const WriteRequest&>(*request_holder);
        request.Process(ts);
    }
}

/*
vector<RequestHolder> ReadReadRequests(istream& in_stream) {
    const size_t request_count = ReadNumberOnLine<size_t>(in_stream);

    vector<RequestHolder> requests;
    requests.reserve(request_count);

    for (size_t i = 0; i < request_count; ++i) {
        string request_str;
        getline(in_stream, request_str);
        if (auto request = ParseReadRequest(request_str)) {
            requests.push_back(move(request));
        }
    }
    return requests;
}
*/

Json::Node ProcessReadRequests(const vector<RequestHolder>& requests, TransportSystem& ts) {
    vector<Json::Node> responses;
    for (const auto& request_holder : requests) {
        const auto& request = static_cast<const ReadRequest<Json::Node>&>(*request_holder);
        responses.push_back(request.Process(ts));
    }
    return Json::Node(responses);
}

void PrintResponses(const Json::Node& responses, ostream& stream) {
    stream << responses;
}

