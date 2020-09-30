#include "transport_system.h"
#include "geo.h"

using namespace std;

double CalculateGeoDistance(shared_ptr<Stop> left, shared_ptr<Stop> right) {
    return CalculateGeoDistance(left->lat, left->lon, right->lat, right->lon);
}

double CalculateStopsDistance(shared_ptr<Stop> left, shared_ptr<Stop> right) {
    double distance = CalculateGeoDistance(left, right);
    if (right->distances.count(left->name)) {
        distance = right->distances[left->name];
    }
    if (left->distances.count(right->name)) {
        distance = left->distances[right->name];
    }
    return distance;
}

void RoundBus::AddRouteToGraph(unique_ptr<Graph::DirectedWeightedGraph<double>>& graph, double velocity, unordered_map<size_t, Json::Node>& edges_description) const {
    for (int i = 0; i < stops.size(); ++i) {
        double distance = 0.0;
        for (int j = i + 1; j < stops.size(); ++j) {
            distance += CalculateStopsDistance(stops[j - 1], stops[j]);
            map<string, Json::Node> edge_description;
            edge_description["type"] = Json::Node(string("Bus"));
            edge_description["bus"] = Json::Node(name);
            edge_description["span_count"] = Json::Node(double(j - i));
            edge_description["time"] = distance / velocity;
            edges_description[graph->AddEdge({stops[i]->id * 2 + 1, stops[j]->id * 2, distance / velocity})]
                = edge_description;
        }
    }
}

void StraightBus::AddRouteToGraph(unique_ptr<Graph::DirectedWeightedGraph<double>>& graph, double velocity, unordered_map<size_t, Json::Node>& edges_description) const {
    for (int i = 0; i < stops.size(); ++i) {
        {
            double distance = 0.0;
            for (int j = i + 1; j < stops.size(); ++j) {
                distance += CalculateStopsDistance(stops[j - 1], stops[j]);
                map<string, Json::Node> edge_description;
                edge_description["type"] = Json::Node(string("Bus"));
                edge_description["bus"] = Json::Node(name);
                edge_description["span_count"] = Json::Node(double(j - i));
                edge_description["time"] = distance / velocity;
                edges_description[graph->AddEdge({stops[i]->id * 2 + 1, stops[j]->id * 2, distance / velocity})]
                    = edge_description;
            }
        }
        {
            double distance = 0.0;
            for (int j = i - 1; j >= 0; --j) {
                distance += CalculateStopsDistance(stops[j + 1], stops[j]);
                map<string, Json::Node> edge_description;
                edge_description["type"] = Json::Node(string("Bus"));
                edge_description["bus"] = Json::Node(name);
                edge_description["span_count"] = Json::Node(double(i - j));
                edge_description["time"] = distance / velocity;
                edges_description[graph->AddEdge({stops[i]->id * 2 + 1, stops[j]->id * 2, distance / velocity})]
                    = edge_description;
            }
        }
    }
}

shared_ptr<Stop> TransportSystem::AddDummyStop(const string& stop_name) {
    if (auto it = name_to_stop_.find(stop_name); it != name_to_stop_.end()) {
        return it->second;
    }
    stops_.push_back(make_shared<Stop>(stop_name, 0, 0, stops_.size(), unordered_map<string, double>()));
    name_to_stop_[stops_.back()->name] = stops_.back();
    return stops_.back();
}

shared_ptr<Stop> TransportSystem::AddStop(const string& stop_name, double lat, double lon, unordered_map<string, double> distances) {
    if (auto it = name_to_stop_.find(stop_name); it != name_to_stop_.end()) {
        it->second->lat = lat;
        it->second->lon = lon;
        it->second->distances = distances;
        return it->second;
    }
    stops_.push_back(make_shared<Stop>(stop_name, lat, lon, stops_.size(), distances));
    name_to_stop_[stops_.back()->name] = stops_.back();
    return stops_.back();
}

shared_ptr<Stop> TransportSystem::GetStop(Stop::ID id) const {
    return stops_.at(id);
}

shared_ptr<Stop> TransportSystem::GetStop(const string& stop_name) const {
    if (name_to_stop_.find(stop_name) == name_to_stop_.end()) {
        return nullptr;
    }
    return name_to_stop_.at(stop_name);
}

vector<shared_ptr<Stop>> TransportSystem::AddDummyStops(const vector<string>& route) {
    vector<shared_ptr<Stop>> stops;

    for (const auto& stop_name : route) {
        stops.push_back(AddDummyStop(stop_name));
    }

    return stops;
}

optional<vector<shared_ptr<Bus>>> TransportSystem::GetBusesOnStop(const string& stop_name) const {
    if (!stop_to_buses_.count(stop_name)) {
        return nullopt;
    }
    return stop_to_buses_.at(stop_name);
}

shared_ptr<Bus> TransportSystem::AddRoundBus(const std::string& bus_name, const vector<string>& route) {
    buses_.push_back(make_shared<RoundBus>(AddDummyStops(route), bus_name, buses_.size()));
    name_to_bus_[buses_.back()->name] = buses_.back();
    for (const auto& stop : route) {
        stop_to_buses_[stop].push_back(buses_.back());
    }
    return buses_.back();
}

shared_ptr<Bus> TransportSystem::AddStraightBus(const std::string& bus_name, const vector<string>& route) {
    buses_.push_back(make_shared<StraightBus>(AddDummyStops(route), bus_name, buses_.size()));
    name_to_bus_[buses_.back()->name] = buses_.back();
    for (const auto& stop : route) {
        stop_to_buses_[stop].push_back(buses_.back());
    }
    return buses_.back();
}

shared_ptr<Bus> TransportSystem::GetBus(Bus::ID id) const {
    return buses_.at(id);
}

shared_ptr<Bus> TransportSystem::GetBus(const string& bus_name) const {
    if (name_to_bus_.find(bus_name) == name_to_bus_.end()) {
        return nullptr;
    }
    return name_to_bus_.at(bus_name);
}

void TransportSystem::BuildGraph() {
    graph_ = make_unique<Graph::DirectedWeightedGraph<double>>(2 * stops_.size());
    edges_description.clear();
    for (size_t v = 0; v < stops_.size(); ++v) {
        map<string, Json::Node> edge_description;
        edge_description["type"] = Json::Node(string("Wait"));
        edge_description["stop_name"] = Json::Node(stops_[v]->name);
        edge_description["time"] = Json::Node(WaitTime);
        edges_description[graph_->AddEdge({v * 2 , v * 2 + 1, WaitTime})] = Json::Node(edge_description);
    }
    for (const auto& bus : buses_) {
        bus->AddRouteToGraph(graph_, Velocity, edges_description);
    }
    router = make_unique<Graph::Router<double>>(*graph_.get());
}
