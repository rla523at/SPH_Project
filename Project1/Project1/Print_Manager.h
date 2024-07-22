#pragma once
#include <sstream>
#include <iostream>

class Print_Manager
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

    _stream = std::stringstream();
    _can_record = false;
  }

private:
  static inline bool              _can_record = false;
  static inline std::stringstream _stream;
};