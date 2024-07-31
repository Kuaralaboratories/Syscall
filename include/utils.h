#ifndef SYSCALL_UTILS_H
#define SYSCALL_UTILS_H

#define varOrReturn(var, init)                                                 \
  auto var = (init);                                                           \
  if (!var)                                                                    \
    return nullptr;

#include <optional>
#include <string>
#include <iostream>

namespace syscall {

struct SourceFile {
  std::string_view path;
  std::string buffer;
};

struct SourceLocation {
  std::string_view filepath;
  int line;
  int col;
};

std::nullptr_t report(SourceLocation location,
                      std::string_view message,
                      bool isWarning = false) {
  if (isWarning) {
    std::cerr << "Warning: ";
  } else {
    std::cerr << "Error: ";
  }
  std::cerr << location.filepath << ":" << location.line << ":" << location.col << " - " << message << std::endl;
  return nullptr;
}

template <typename Ty> class ConstantValueContainer {
  std::optional<Ty> value = std::nullopt;

public:
  void setConstantValue(std::optional<Ty> val) { value = std::move(val); }
  std::optional<Ty> getConstantValue() const { return value; }
};
} // namespace syscall

#endif // SYSCALL_UTILS_H
