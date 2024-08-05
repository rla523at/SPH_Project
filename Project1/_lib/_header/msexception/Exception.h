#pragma once
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <cassert>

#define FILE_NAME ms::exception::extract_file_name(__FILE__)
#define REQUIRE_ALWAYS(requirement, message) ms::exception::require_exit(requirement, message, FILE_NAME, __FUNCTION__, __LINE__)

#ifdef _DEBUG
#define REQUIRE(requirement, message) ms::exception::require(requirement, message, FILE_NAME, __FUNCTION__, __LINE__)
#define REQUIRE_EX(requirement, message) ms::exception::require_exception(requirement, message, FILE_NAME, __FUNCTION__, __LINE__)
#define EXCEPTION(message) ms::exception::require_exit(false, message, FILE_NAME, __FUNCTION__, __LINE__)
#else
#define REQUIRE(requirement, message)
#define REQUIRE_EX(requirement, message)
#define EXCEPTION(message)
#endif

namespace ms::exception
{
inline void require(const bool requirement, const std::string_view& message, const std::string_view& file_name, const std::string_view& function_name, const int num_line)
{
  if (!requirement)
  {
    std::cout << "\n==============================EXCEPTION========================================\n";
    std::cout << "File\t\t: " << file_name << "\n";
    std::cout << "Function\t: " << function_name << "\n";
    std::cout << "Line\t\t: " << num_line << "\n";
    std::cout << "Message\t\t: " << message.data() << "\n";
    std::cout << "==============================EXCEPTION========================================\n\n";

    assert(false);
  }
}

inline void require(const bool requirement, const std::wstring_view& message, const std::string_view& file_name, const std::string_view& function_name, const int num_line)
{
  if (!requirement)
  {
    std::wcout << "\n==============================EXCEPTION========================================\n";
    std::wcout << "File\t\t: " << file_name.data() << "\n";
    std::wcout << "Function\t: " << function_name.data() << "\n";
    std::wcout << "Line\t\t: " << num_line << "\n";
    std::wcout << "Message\t\t: " << message.data() << "\n";
    std::wcout << "==============================EXCEPTION========================================\n\n";

    assert(false);
  }
}

inline void require_exception(const bool requirement, const std::string_view& message, const std::string_view& file_name, const std::string_view& function_name, const int num_line)
{
  if (!requirement)
  {
    std::ostringstream os;

    os << "\n==============================EXCEPTION========================================\n";
    os << "File\t\t: " << file_name << "\n";
    os << "Function\t: " << function_name << "\n";
    os << "Line\t\t: " << num_line << "\n";
    os << "Message\t\t: " << message.data() << "\n";
    os << "==============================EXCEPTION========================================\n\n";

    throw std::runtime_error(os.str());
  }
}

inline void require_exit(const bool requirement, const std::string_view& message, const std::string_view& file_name, const std::string_view& function_name, const int num_line)
{
  if (!requirement)
  {
    std::wcout << "\n==============================EXCEPTION========================================\n";
    std::wcout << "File\t\t: " << file_name.data() << "\n";
    std::wcout << "Function\t: " << function_name.data() << "\n";
    std::wcout << "Line\t\t: " << num_line << "\n";
    std::wcout << "Message\t\t: " << message.data() << "\n";
    std::wcout << "==============================EXCEPTION========================================\n\n";

    exit(523);
  }
}

inline void require_exit(const bool requirement, const std::wstring_view& message, const std::string_view& file_name, const std::string_view& function_name, const int num_line)
{
  if (!requirement)
  {
    std::wcout << "\n==============================EXCEPTION========================================\n";
    std::wcout << "File\t\t: " << file_name.data() << "\n";
    std::wcout << "Function\t: " << function_name.data() << "\n";
    std::wcout << "Line\t\t: " << num_line << "\n";
    std::wcout << "Message\t\t: " << message.data() << "\n";
    std::wcout << "==============================EXCEPTION========================================\n\n";

    exit(523);
  }
}

inline std::string_view extract_file_name(std::string_view __FILE__macro)
{
  const auto num_remove = __FILE__macro.rfind("\\") + 1;
  __FILE__macro.remove_prefix(num_remove);

  return __FILE__macro;
}
} // namespace ms::exception