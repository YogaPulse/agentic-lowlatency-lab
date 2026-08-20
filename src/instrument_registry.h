#pragma once

#include <map>
#include <optional>
#include <string>
#include <string_view>

enum class RegisterInstrumentResult : std::uint8_t
{
    Registered,
    NameAlreadyExists,
    IdAlreadyExists
};

class InstrumentRegistry
{
public:
    [[nodiscard]] RegisterInstrumentResult register_instrument(
        std::string_view name,
        std::uint32_t id);

    [[nodiscard]] std::optional<std::uint32_t> id_for(
        std::string_view name) const noexcept;
    [[nodiscard]] std::optional<std::string_view> name_for(
        std::uint32_t id) const noexcept;

private:
    std::map<std::string, std::uint32_t, std::less<>> _ids_by_name;
    std::map<std::uint32_t, std::string> _names_by_id;
};
