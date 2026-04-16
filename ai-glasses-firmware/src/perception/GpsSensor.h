#pragma once

#include <optional>
#include <string>

namespace perception {

struct GpsData {
    double latitude;
    double longitude;
    double altitude;
    int satellites;
};

class GpsSensor {
public:
    explicit GpsSensor(const std::string& device_path = "mock");
    ~GpsSensor();

    GpsSensor(const GpsSensor&) = delete;
    GpsSensor& operator=(const GpsSensor&) = delete;

    std::optional<GpsData> readLocation();
    bool isUsingMock() const;

private:
    void generateMockData();

    bool use_mock_;
    double mock_lat_;
    double mock_lon_;
    double mock_angle_;
};

} // namespace perception
