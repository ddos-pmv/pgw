#include "pgw_utils/unique_fd.h"

#include <unistd.h>

#include <type_traits>
#include <utility>

namespace protei
{
  UniqueFd::UniqueFd(int fd) : fd_(fd) {}
  UniqueFd::~UniqueFd()
  {
    if (fd_ >= 0)
    {
      close(fd_);
    }
  }

  UniqueFd::UniqueFd(UniqueFd &&other) noexcept : fd_(std::exchange(other.fd_, -1)) {}

  UniqueFd &UniqueFd::operator=(UniqueFd &&other)
  {
    if (this != &other)
    {
      reset(std::exchange(other.fd_, -1));
    }
    return *this;
  }

  int UniqueFd::release()
  {
    return std::exchange(fd_, -1);
  }

  int UniqueFd::get() const
  {
    return fd_;
  }

  //     template <std::integral T>
  //     auto operator<=>(T other) const
  //     {
  //       return fd_ <=> static_cast<long long>(other);
  //     }

  //     template <std::integral T>
  //     bool operator==(T other) const
  //     {
  //       return fd_ == static_cast<int>(other);
  //     }

  //     auto operator<=>(const UniqueFd &other) const = default;

  //   private:
  //     int fd_;

  void UniqueFd::reset(int fd)
  {
    close(fd_);
    fd_ = fd;
  }

} // namespace protei