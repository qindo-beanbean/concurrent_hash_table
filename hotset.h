#ifndef HOTSET_H
#define HOTSET_H

#include <random>

// Simple hot-set generator: p_hot probability to choose from [0, hotN),
// otherwise choose from [hotN, universe).
class HotsetGen {
public:
    HotsetGen(int universe, int hotN, double p_hot, uint32_t seed)
        : universe_(universe), hotN_(hotN), p_hot_(p_hot),
          gen_(seed),
          dist_hot_(0, hotN_ - 1),
          dist_cold_(hotN_, universe_ - 1),
          coin_(0.0, 1.0) {}

    int draw() {
        if (coin_(gen_) < p_hot_) return dist_hot_(gen_);
        return dist_cold_(gen_);
    }

private:
    int universe_;
    int hotN_;
    double p_hot_;
    std::mt19937 gen_;
    std::uniform_int_distribution<int> dist_hot_;
    std::uniform_int_distribution<int> dist_cold_;
    std::uniform_real_distribution<double> coin_;
};

#endif