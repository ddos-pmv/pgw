#pragma once
#include <unistd.h>

#include <type_traits>
#include <utility>

namespace protei
{
  class UniqueFd
  {
  public:
    explicit UniqueFd(int fd);
    ~UniqueFd();

    UniqueFd(const UniqueFd &) = delete;
    UniqueFd &operator=(const UniqueFd &) = delete;

    UniqueFd(UniqueFd &&other) noexcept;

    UniqueFd &operator=(UniqueFd &&other);

    int release();

    int get() const;

    template <std::integral T>
    auto operator<=>(T other) const
    {
      return fd_ <=> static_cast<long long>(other);
    }

    template <std::integral T>
    bool operator==(T other) const
    {
      return fd_ == static_cast<int>(other);
    }

    auto operator<=>(const UniqueFd &other) const = default;

  private:
    int fd_;

    void reset(int fd = -1);
  };

} // namespace protei