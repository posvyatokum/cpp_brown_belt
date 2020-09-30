#pragma once
#include <cmath>

inline double deg2rad(double deg) {
    static const double pi = 3.1415926535;
    return deg * pi / 180.0;
}

inline double CalculateGeoDistance(double lat1, double lon1, double lat2, double lon2) {
    static const double R = 6371'000;

    double dLat = deg2rad(lat2-lat1);  // deg2rad below
    double dLon = deg2rad(lon2-lon1); 
    double a = 
        sin(dLat/2) * sin(dLat/2) +
        cos(deg2rad(lat1)) * cos(deg2rad(lat2)) * 
        sin(dLon/2) * sin(dLon/2)
        ; 
    double c = 2 * atan2(sqrt(a), sqrt(1-a)); 
    double d = R * c;
    return d;

}
