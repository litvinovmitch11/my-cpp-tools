#include <vector>
#include <algorithm>

template <typename T>
T sum(const std::vector<T>& v) {
    T total = 0;
    std::for_each(v.begin(), v.end(), [&](T x) { total += x; });
    return total;
}

int main() {
    std::vector<int> nums = {1, 2, 3};
    return sum(nums);
}
