#include <iostream>

#include "finenav_ndt_octomap/ndt_octomap.hpp"

int main(int argc, char** argv) {
    if (argc != 2) return 1;
    finenav::ndt::NdtOctomap map;
    ndt_octomap::Params config;
    config.resolution = 0.1;
    config.max_depth = 7;
    map.configure(config);
    for (int x = -20; x < 20; ++x) {
        for (int y = -10; y < 10; ++y) {
            finenav::ndt::NdtCell cell;
            cell.mean = {(x + 0.5) * 0.1, (y + 0.5) * 0.1, 0.0};
            cell.mean.z() = 1.0 + 0.25 * cell.mean.x();
            cell.covariance << 0.001, 0.0, 0.00025,
                               0.0, 0.001, 0.0,
                               0.00025, 0.0, 0.0000635;
            cell.sample_count = 40;
            if (!map.setDistribution(cell.mean, cell)) return 1;
        }
    }
    if (!map.save(argv[1])) {
        std::cerr << "Cannot save test NDT map\n";
        return 1;
    }
    return 0;
}
