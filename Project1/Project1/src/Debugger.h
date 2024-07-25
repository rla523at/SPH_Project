#pragma once
#include <algorithm>
#include <iostream>
#include <sstream>

namespace ms
{
class Debugger
{
public:
  static bool can_record(void)
  {
    return _can_record;
  }

  static void start_record(void)
  {
    _can_record = true;
  }

  static std::stringstream& record(void)
  {
    return _stream;
  }

  static void print(void)
  {
    if (_can_record)
      std::cout << _stream.str();

    _stream     = std::stringstream();
    _can_record = false;
  }

private:
  static inline bool              _can_record = false;
  static inline std::stringstream _stream;
};

} // namespace ms

namespace ms
{

//template <typename T>
//void print_sort_and_count(const std::vector<T>& values)
//{
//  auto sorted_values = values;
//  std::sort(sorted_values.begin(), sorted_values.end());
//
//  const auto num_value = values.size();
//
//  T      value = sorted_values[0];
//  size_t count = 1;
//  for (size_t i = 1; i < num_value; ++i)
//  {
//    if (value != sorted_values[i])
//    {
//      std::cout << value << " " << count << "\n";
//
//      value = sorted_values[i];
//      count = 1;
//    }
//    else
//      count++;
//  }
//
//  std::cout << value << " " << count << "\n";
//}

template <typename T>
void print_sort_and_count(const std::vector<T>& values, const T epsilon = 0)
{
  auto sorted_values = values;
  std::sort(sorted_values.begin(), sorted_values.end());

  const auto num_value = values.size();

  T      value = sorted_values[0];
  size_t count = 1;
  for (size_t i = 1; i < num_value; ++i)
  {
    if (epsilon < sorted_values[i] - value)
    {
      std::cout << value << " " << count << "\n";

      value = sorted_values[i];
      count = 1;
    }
    else
      count++;
  }

  std::cout << value << " " << count << "\n";
}

template <typename T>
void print_min_max(const std::vector<T>& values)
{
  auto [min_val, max_val] = std::minmax_element(values.begin(),values.end());

  std::cout << "min_val : " << *min_val << "\nmax_val : " << *max_val << "\n";
}

template <typename T>
void print(const T& v)
{
  std::cout << v << "\n";
}

template <>
void print(const Vector3& v)
{
  std::cout << v.x << " " << v.y << " " << v.z << "\n";
}


bool is_nan(const Vector3& v)
{
  return v.x != v.x || v.y != v.y || v.z != v.z;
}

} // namespace ms