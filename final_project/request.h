#pragma once
#include "transport_system.h"
#include "parser.h"
#include "json.h"

#include <string>
#include <memory>
#include <vector>
#include <string_view>
#include <utility>

struct Request;
using RequestHolder = std::shared_ptr<Request>;

struct Request {
    enum Type {
        ADD_STOP = 0,
        ADD_ROUND_BUS = 1,
        ADD_STRAIGHT_BUS = 2,
        ADD_PARAMS = 3,
        READ_BUS = 4,
        READ_STOP = 5,
        READ_ROUTE = 6
    };

    Request(Type type) : type(type) {}
    static RequestHolder Create(Type type);
    virtual void ParseFrom(const Json::Node& node) = 0;
    virtual ~Request() = default;

    const Type type;
};

struct WriteRequest : Request {
    using Request::Request;
    virtual void Process(TransportSystem& ts) const = 0;
};

template<typename ResultType>
struct ReadRequest : Request {
    using Request::Request;
    virtual ResultType Process(const TransportSystem& ts) const = 0;
    int64_t request_id;
};

struct AddParamsRequest : WriteRequest {
    AddParamsRequest(): WriteRequest(Type::ADD_PARAMS) {}
    void ParseFrom(const Json::Node& node) override;
    void Process(TransportSystem& ts) const override;

    double wait_time, velocity;
};

struct AddStopRequest : WriteRequest {
    AddStopRequest(): WriteRequest(Type::ADD_STOP) {}
    void ParseFrom(const Json::Node& node) override;
    void Process(TransportSystem& ts) const override;

    std::string stop_name;
    double lat, lon;
    std::unordered_map<std::string, double> distances;
};

struct AddBusRequest : WriteRequest {
    using WriteRequest::WriteRequest;
    std::string bus_name;
    std::vector<std::string> stops;
};

struct AddRoundBusRequest : AddBusRequest {
    AddRoundBusRequest(): AddBusRequest(Type::ADD_ROUND_BUS) {}
    void ParseFrom(const Json::Node& node) override;
    void Process(TransportSystem& ts) const override;

};

struct AddStraightBusRequest : AddBusRequest {
    AddStraightBusRequest(): AddBusRequest(Type::ADD_STRAIGHT_BUS) {}
    void ParseFrom(const Json::Node& node) override;
    void Process(TransportSystem& ts) const override;

};

struct ReadBusRequest : ReadRequest<Json::Node> {
    ReadBusRequest(): ReadRequest(Type::READ_BUS) {}
    void ParseFrom(const Json::Node& node) override;
    Json::Node Process(const TransportSystem& ts) const override;

    std::string bus_name;
};

struct ReadStopRequest : ReadRequest<Json::Node> {
    ReadStopRequest(): ReadRequest(Type::READ_STOP) {}
    void ParseFrom(const Json::Node& node) override;
    Json::Node Process(const TransportSystem& ts) const override;

    std::string stop_name;
};

struct ReadRouteRequest : ReadRequest<Json::Node> {
    ReadRouteRequest(): ReadRequest(Type::READ_ROUTE) {}
    void ParseFrom(const Json::Node& node) override;
    Json::Node Process(const TransportSystem& ts) const override;

    std::string from, to;
};

RequestHolder ParseWriteRequest(const Json::Node& request_json);
RequestHolder ParseReadRequest(const Json::Node& request_json);

std::pair<std::vector<RequestHolder>, std::vector<RequestHolder>> ReadRequests(std::istream& in_stream = std::cin);
// std::vector<RequestHolder> ReadWriteRequests(std::istream& in_stream = std::cin);
void ProcessWriteRequests(const std::vector<RequestHolder>& requests, TransportSystem& ts);
//std::vector<RequestHolder> ReadReadRequests(std::istream& in_stream = std::cin);
Json::Node ProcessReadRequests(const std::vector<RequestHolder>& requests, TransportSystem& ts);
void PrintResponses(const Json::Node& responses, std::ostream& stream = std::cout);

