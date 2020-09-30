#pragma once
#include "router.h"
#include "json.h"
#include <unordered_map>
#include <set>
#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include <optional>

struct Stop {
    using ID = size_t;
    std::string name;
    double lat, lon;
    ID id;
    std::unordered_map<std::string, double> distances;

    Stop(std::string name, double lat, double lon, ID id, std::unordered_map<std::string, double> distances):
        name(name),
        lat(lat),
        lon(lon),
        id(id),
        distances(distances)
        {}
};

double CalculateGeoDistance(std::shared_ptr<Stop> left, std::shared_ptr<Stop> right);
double CalculateStopsDistance(std::shared_ptr<Stop> left, std::shared_ptr<Stop> right);

struct Bus {
    using ID = size_t;
    std::vector<std::shared_ptr<Stop>> stops;
    std::string name;
    ID id;
    std::set<ID> stops_set;

    Bus(
        const std::vector<std::shared_ptr<Stop>>& stops,
        const std::string& name,
        ID id
    )
        : stops(stops)
        , name(name)
        , id(id)
    {
        for (const auto& stop : stops) {
            stops_set.insert(stop->id);
        }
    }

    virtual size_t StopsCount() const = 0;
    size_t UniqueStopsCount() const {
        return stops_set.size();
    }
    virtual double GeoRouteLength() const  = 0;
    virtual double RouteLength() const  = 0;
    double Curvature() const {
        return RouteLength() / GeoRouteLength();
    }

    virtual void AddRouteToGraph(std::unique_ptr<Graph::DirectedWeightedGraph<double>>& graph, double velocity, std::unordered_map<size_t, Json::Node>& edges_description) const = 0;
};

struct RoundBus : Bus {
    RoundBus(
        const std::vector<std::shared_ptr<Stop>>& stops,
        const std::string& name,
        ID id
    )
        : Bus(stops, name, id)
        {}

    size_t StopsCount() const override {
        return stops.size();
    }

    double GeoRouteLength() const override {
        double result = CalculateGeoDistance(stops.back(), stops[0]);
        for (size_t i = 1; i < stops.size(); ++i) {
            result += CalculateGeoDistance(stops[i - 1], stops[i]);
        }
        return result;
    }

    double RouteLength() const override {
        double result = CalculateStopsDistance(stops.back(), stops[0]);
        for (size_t i = 1; i < stops.size(); ++i) {
            result += CalculateStopsDistance(stops[i - 1], stops[i]);
        }
        return result;
    }

    virtual void AddRouteToGraph(std::unique_ptr<Graph::DirectedWeightedGraph<double>>& graph, double velocity, std::unordered_map<size_t, Json::Node>& edges_description) const override;
};

struct StraightBus : Bus {
    StraightBus(
        const std::vector<std::shared_ptr<Stop>>& stops,
        const std::string& name,
        ID id
    )
        : Bus(stops, name, id)
        {}

    size_t StopsCount() const override {
        return 2 * stops.size() - 1;
    }

    double GeoRouteLength() const override {
        double result = 0;
        for (size_t i = 1; i < stops.size(); ++i) {
            result += CalculateGeoDistance(stops[i - 1], stops[i]);
        }
        return result * 2;
    }

    double RouteLength() const override {
        double result = 0;
        for (size_t i = 1; i < stops.size(); ++i) {
            result += CalculateStopsDistance(stops[i - 1], stops[i]);
            result += CalculateStopsDistance(stops[i], stops[i - 1]);
        }
        return result;
    }

    virtual void AddRouteToGraph(std::unique_ptr<Graph::DirectedWeightedGraph<double>>& graph, double velocity, std::unordered_map<size_t, Json::Node>& edges_description) const override;
};

class TransportSystem {
private:
    double WaitTime = 0.0;
    double Velocity = 1.0;

    std::vector<std::shared_ptr<Stop>> stops_;
    std::unordered_map<std::string, std::shared_ptr<Stop>> name_to_stop_;

    std::vector<std::shared_ptr<Bus>> buses_;
    std::unordered_map<std::string, std::shared_ptr<Bus>> name_to_bus_;

    std::unordered_map<std::string, std::vector<std::shared_ptr<Bus>>> stop_to_buses_;

    std::unique_ptr<Graph::DirectedWeightedGraph<double>> graph_;
    std::unordered_map<size_t, Json::Node> edges_description;

public:
    std::unique_ptr<Graph::Router<double>> router;

public:
    void SetParams(double wait_time, double velocity) {
        WaitTime = wait_time;
        Velocity = velocity;
    }
    double GetWaitTime() const {
        return WaitTime;
    }
    double GetVelocity() const {
        return Velocity;
    }
    Json::Node GetEdgeDescription(size_t id) const {
        return edges_description.at(id);
    }

    std::shared_ptr<Stop> AddDummyStop(const std::string& stop_name);
    std::shared_ptr<Stop> AddStop(const std::string& stop_name, double lat, double lon,
                                  std::unordered_map<std::string, double> distances = {});
    std::shared_ptr<Stop> GetStop(Stop::ID id) const;
    std::shared_ptr<Stop> GetStop(const std::string& stop_name) const;

    std::optional<std::vector<std::shared_ptr<Bus>>> GetBusesOnStop(const std::string& stop_name) const;

    std::shared_ptr<Bus> AddRoundBus(const std::string& bus_name, const std::vector<std::string>& route);
    std::shared_ptr<Bus> AddStraightBus(const std::string& bus_name, const std::vector<std::string>& route);
    std::shared_ptr<Bus> GetBus(Bus::ID id) const;
    std::shared_ptr<Bus> GetBus(const std::string& bus_name) const;

    void BuildGraph();
private:
    std::vector<std::shared_ptr<Stop>> AddDummyStops(const std::vector<std::string>& route);
};
