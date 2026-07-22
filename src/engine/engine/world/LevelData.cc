#include "engine/world/LevelData.h"

#include <charconv>
#include <fstream>
#include <limits>

#define ASSERT_INVALID_SYNTAX(_expression)                                                         \
    ASSERT((_expression), "Invalid syntax at line {}", lineNumber);

namespace eng::intl {

enum class Header
{
    None,
    Meta,
    Resources,
    TerrainDefs,
    ActorDefs,
    Terrain,
    Actors
};

template <typename T>
T ReadNumberFromString(const std::string_view& str, unsigned lineNumber)
{
    T value     = static_cast<T>(0);
    auto result = std::from_chars(str.data(), str.data() + str.size(), value);
    ASSERT_INVALID_SYNTAX(result.ec == std::errc{});
    return value;
}

std::string_view HeaderToString(Header header)
{
    switch (header)
    {
    case Header::None:        return "None";
    case Header::Meta:        return "Meta";
    case Header::Resources:   return "Resources";
    case Header::TerrainDefs: return "TerrainDefs";
    case Header::ActorDefs:   return "ActorDefs";
    case Header::Terrain:     return "Terrain";
    case Header::Actors:      return "Actors";
    }
}

constexpr std::string_view StrippedString(const std::string_view& str,
                                          const std::string_view& strip = " ")
{
    size_t start = str.find_first_not_of(strip);
    if (start == std::string_view::npos)
        return ""; // Entirely stripped

    size_t end = str.find_last_not_of(strip);
    return str.substr(start, end - start + 1);
}

} // namespace eng::intl
namespace eng {

static constexpr unsigned MaxBufferLength = 1024;

LevelData::LevelData(const std::string_view& path)
{
    std::ifstream stream(path.data());
    ASSERT(stream.is_open(), "Failed to open level data file at '{}'", path);

    intl::Header currentHeader = intl::Header::None;
    unsigned yIndex            = std::numeric_limits<unsigned>::max();
    unsigned zIndex            = 0;

    char lineBuffer[MaxBufferLength];
    for (unsigned lineNumber = 0; stream.getline(lineBuffer, MaxBufferLength); lineNumber++)
    {
        std::string_view line(lineBuffer);

        // Skip empty lines and comments
        if (line.empty() || line.starts_with("#"))
            continue;

        // Check if header
        unsigned prevZIndex = yIndex;
        intl::Header header = ReadHeader(line, yIndex, lineNumber);
        if (header != intl::Header::None)
        {
            currentHeader = header;
            zIndex        = 0;
            // Make sure to not add layers for every header
            if (prevZIndex != yIndex)
            {
                layers.emplace_back<Layer>({
                    .yIndex  = yIndex,
                    .terrain = {},
                    .actors  = {},
                });
                // reserve tilemap vector size
                layers.back().terrain.reserve(width * length);
            }
            continue;
        }

        switch (currentHeader)
        {
        case intl::Header::Meta:        ReadMetaData(line, lineNumber); break;
        case intl::Header::Resources:   break;
        case intl::Header::TerrainDefs: ReadDefinitions(tileDefinitions, line, lineNumber); break;
        case intl::Header::ActorDefs:   ReadDefinitions(actorDefinitions, line, lineNumber); break;
        case intl::Header::Terrain:     ReadTerrainLocation(line, zIndex, lineNumber); break;
        case intl::Header::Actors:      ReadActorLocation(line, zIndex, lineNumber); break;
        default:                        FATAL("Something went wrong, this should never be called");
        }
    }
}

intl::Header LevelData::ReadHeader(const std::string_view& line, unsigned& zIndex,
                                   unsigned lineNumber)
{
    // Check if of header format [...]
    if (!line.starts_with('[') || !line.ends_with(']'))
        return intl::Header::None;

    // Compare name with Header Type
    std::string_view header = line.substr(1, line.length() - 2);
    size_t layerSeparator   = header.find(':');

    if (layerSeparator != std::string_view::npos)
    {
        ASSERT(layerSeparator != header.length() - 1,
               "Must define a number after ':' at line {}",
               lineNumber);

        // get z index number as string
        std::string_view zIndexStr = header.substr(layerSeparator + 1);
        ASSERT(zIndexStr.find_first_not_of("0123456789") == std::string_view::npos,
               "Z index layer must be a number and cannot be negative at line {}",
               lineNumber);

        // Set z index
        unsigned prevZIndex = zIndex;
        zIndex              = std::stoi(header.data() + layerSeparator + 1);
        ASSERT(zIndex == prevZIndex || zIndex - prevZIndex == 1,
               "Z index layer's must be in order at line {}, {}:{}",
               lineNumber,
               prevZIndex,
               zIndex);

        // Set header value
        std::string_view name = header.substr(0, layerSeparator);
        if (name == "Terrain")
            return intl::Header::Terrain;
        if (name == "Actors")
            return intl::Header::Actors;

        FATAL("Syntax error occurred at line {}\n'{}'", lineNumber, line);
    }

    if (header == "Meta")
        return intl::Header::Meta;
    if (header == "Resources")
        return intl::Header::Resources;
    if (header == "TerrainDefs")
        return intl::Header::TerrainDefs;
    if (header == "ActorDefs")
        return intl::Header::ActorDefs;
    return intl::Header::None;
}

void LevelData::ReadMetaData(const std::string_view& line, unsigned lineNumber)
{
    const auto [fieldName, fieldValue] = GetFieldEntry(line, lineNumber);
    if (fieldName == "name")
    {
        name = intl::StrippedString(fieldValue, "\" ");
        return;
    }
    else if (fieldName == "tilemapClass")
    {
        tilemapClassName = intl::StrippedString(fieldValue, "\" ");
        return;
    }

    // Make sure the following is a positive int or float
    bool is_unsigned = fieldValue.find_first_not_of("0123456789") == std::string_view::npos;
    bool is_float    = fieldValue.find_first_not_of(".0123456789") == std::string_view::npos;
    ASSERT_INVALID_SYNTAX(is_unsigned || is_float);

    if (is_unsigned)
    {
        unsigned value = intl::ReadNumberFromString<unsigned>(fieldValue, lineNumber);
        if (fieldName == "width")
            width = value;
        else if (fieldName == "length")
            length = value;
        return;
    }
    ASSERT(is_float,
           "value on line {} must be either a string, unsigned number, or float",
           lineNumber);

    float value = intl::ReadNumberFromString<float>(fieldValue, lineNumber);
    if (fieldName == "cellWidth")
        cellWidth = value;
    else if (fieldName == "cellHeight")
        cellHeight = value;
}

void LevelData::ReadDefinitions(std::unordered_map<unsigned, LevelTypeDef>& definitions,
                                const std::string_view& line, unsigned lineNumber)
{
    const auto [field, data] = GetFieldEntry(line, lineNumber);
    ASSERT_INVALID_SYNTAX(field.length() == 1);

    // Find where | starts if any
    size_t fieldArray = data.find_first_of('|');

    // Split and pass into readMultiFieldData
    std::string_view className = intl::StrippedString(data.substr(0, fieldArray), "\" ");
    LevelTypeDef::UnorderedMap fields;

    if (fieldArray != std::string_view::npos)
        ReadFieldArrayData(intl::StrippedString(data.substr(fieldArray + 1)), fields, lineNumber);

    LevelTypeDef def{
        .name   = std::string(className),
        .hash   = GetFnvHash(className),
        .fields = std::move(fields),
    };
    definitions.emplace(static_cast<unsigned>(field[0]), std::move(def));
}

void LevelData::ReadActorLocation(const std::string_view& line, unsigned& zIndex,
                                  unsigned lineNumber)
{
    unsigned x = 0;
    for (char c : line)
    {
        if (c == ' ')
            continue;
        x++;
        if (c == '.')
            continue;

        layers.back().actors.push_back(Layer::Location{
            .x  = x - 1,
            .z  = zIndex,
            .id = static_cast<unsigned>(c),
        });
    }
    zIndex++;
}

void LevelData::ReadTerrainLocation(const std::string_view& line, unsigned& zIndex,
                                    unsigned lineNumber)
{
    unsigned x = 0;
    for (char c : line)
    {
        if (c == ' ')
            continue;
        x++;

        layers.back().terrain.push_back(static_cast<unsigned>(c));
    }
    zIndex++;
}

void LevelData::ReadFieldArrayData(const std::string_view& fieldArrayString,
                                   LevelTypeDef::UnorderedMap& fields, unsigned lineNumber)
{
    // Separate fields by 'field = value' searching for ,
    size_t offset = 0;
    while (offset < fieldArrayString.length())
    {
        size_t end = fieldArrayString.find_first_of(',', offset);

        std::string_view fieldEntry =
            intl::StrippedString(fieldArrayString.substr(offset, end - offset));
        if (fieldEntry.empty())
            break;

        size_t separator = fieldEntry.find_first_of('=');
        ASSERT_INVALID_SYNTAX(separator != std::string_view::npos);

        // Read field
        std::string_view name  = intl::StrippedString(fieldEntry.substr(0, separator));
        std::string_view value = intl::StrippedString(fieldEntry.substr(separator + 1));
        fields.emplace(name, ReadDataField(value, lineNumber));

        // End of fields
        if (end == std::string_view::npos)
            break;
        offset = end + 1;
    }
}

LevelTypeDef::FieldVariant LevelData::ReadDataField(const std::string_view& fieldValue,
                                                    unsigned lineNumber) const
{
    // String
    if (fieldValue.find_first_of('"') != std::string_view::npos)
        return std::string(intl::StrippedString(fieldValue, "\""));
    else if (fieldValue.find_first_of('\'') != std::string_view::npos)
        return fieldValue.at(1);

    // Vector2
    // TODO: ...

    // bool
    bool boolResult = fieldValue == "true";
    if (boolResult || fieldValue == "false")
        return boolResult;

    // Digit
    size_t decimal = fieldValue.find_first_of('.');
    bool isDigit   = fieldValue.find_last_not_of(".-0123456789") == std::string_view::npos;
    if (isDigit)
    {
        // floating point
        if (decimal != std::string_view::npos)
            return intl::ReadNumberFromString<float>(fieldValue, lineNumber);
        // int
        return intl::ReadNumberFromString<int>(fieldValue, lineNumber);
    }

    FATAL("Invalid syntax at line {}. Type invalid.", lineNumber);
}

std::tuple<std::string_view, std::string_view> LevelData::GetFieldEntry(
    const std::string_view& line, unsigned lineNumber) const
{
    size_t setterIndex = line.find_first_of('=');
    ASSERT_INVALID_SYNTAX(setterIndex != std::string_view::npos && setterIndex > 0)

    std::string_view name  = intl::StrippedString(line.substr(0, setterIndex));
    std::string_view value = intl::StrippedString(line.substr(setterIndex + 1));
    return std::make_tuple(name, value);
}

} // namespace eng
