#include "instrument_registry.h"

RegisterInstrumentResult InstrumentRegistry::register_instrument(
    std::string_view name,
    std::uint32_t id)
{
    if (_ids_by_name.contains(name))
    {
        return RegisterInstrumentResult::NameAlreadyExists;
    }
    if (_names_by_id.contains(id))
    {
        return RegisterInstrumentResult::IdAlreadyExists;
    }

    _names_by_id.emplace(id, name);
    _ids_by_name.emplace(name, id);
    return RegisterInstrumentResult::Registered;
}

std::optional<std::uint32_t> InstrumentRegistry::id_for(
    std::string_view name) const noexcept
{
    const auto instrument = _ids_by_name.find(name);
    if (instrument == _ids_by_name.end())
    {
        return std::nullopt;
    }
    return instrument->second;
}

std::optional<std::string_view> InstrumentRegistry::name_for(
    std::uint32_t id) const noexcept
{
    const auto instrument = _names_by_id.find(id);
    if (instrument == _names_by_id.end())
    {
        return std::nullopt;
    }
    return instrument->second;
}
